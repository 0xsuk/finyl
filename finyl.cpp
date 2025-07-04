#include "finyl.h"
#include "error.h"
#include "util.h"
#include "dsp.h"
#include "dev.h"
#include <thread>
#include <alsa/asoundlib.h>
#include <math.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <string_view>
#include <memory>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include "action.h"
#include "controller.h"
#include "interface.h"

App gApp;

Deck::Deck(finyl_deck_type _type): type(_type),
                                   pTrack(nullptr),
                                   action_state(new ActionState{}),
                                   gain(1.0),
                                   gain0(0.0),
                                   gain1(0.0),
                                   mute0(false),
                                   filter1(1.0),
                                   quantize(true),
                                   master(true),
                                   delayState(new DelayState{.wetmix=0.5, .drymix=1.0, .feedback=0.5}),
                                   bqisoState(new BiquadFullKillEQEffectGroupState()), //cant initialize until sample_rate is set
                                   fftState(new FFTState())
{
  buffer.resize(gApp.audio->get_period_size_2());
  for (size_t i = 0; i<MAX_STEMS_SIZE; i++) {
    stem_buffers[i].resize(gApp.audio->get_period_size_2());
  }

  print_deck_name(*this);
  printf("Deck initialized\n");
}

finyl_track_meta::finyl_track_meta(): id(0),
                                      bpm(0),
                                      musickeyid(0),
                                      filesize(0) {};

finyl_track::finyl_track(): meta(),
                            stems_size(0),
                            jump_lock(false),
                            playing(false),
                            loop_active(false),
                            loop_in(-1),
                            loop_out(-1),
                            pitch_scale(1.0),
                            key_shift_semitones(0) {
  std::fill(indxs, indxs + MAX_STEMS_SIZE, 0);
  for (auto&p : stretchers) {
    p = std::make_unique<rb>(gApp.audio->get_sample_rate(), 2, rb::OptionProcessRealTime | rb::OptionEngineFiner, 1.0);
    p->setMaxProcessSize(gApp.audio->max_process_size);
  }
};




int finyl_get_quantized_beat_index(finyl_track& t, int index) {
  if (t.beats.size() <= 0) {
    return -1;
  }
  
  //44100 samples = 1sec
  //index samples = 1 / 44100 * index sec
  //= 1 / 44100 * index * 1000 millisec
  double nowtime = (1000.0 / gApp.audio->get_sample_rate()) * index;

  if (nowtime < t.beats[0].time) {
    return 0;
  }
  if (nowtime > t.beats[t.beats.size()-1].time) {
    return t.beats.size()-1;
  }
  for (int i = 1; i<t.beats.size()-1; i++) {
    if (t.beats[i-1].time <= nowtime && nowtime <= t.beats[i].time) {
      // [i-1]      half       [i]
      //     -margin-  -margin-
      double margin = (t.beats[i].time - t.beats[i-1].time) / 2.0;
      double half = t.beats[i-1].time + margin;
      
      if (nowtime < half) {
        return i-1;
      } else {
        return i;
      }
    }
  }

  return -1;
}

int finyl_get_quantized_time(finyl_track& t, int index) {
  int i = finyl_get_quantized_beat_index(t, index);
  if (i == -1) {
    return -1;
  }
  return t.beats[i].time;
}

double finyl_get_quantized_index(finyl_track& t, int index) {
  double v =  (gApp.audio->get_sample_rate() / 1000.0) * finyl_get_quantized_time(t, index);

  return v;
}

bool file_exist(std::string_view file) {
  if (access(file.data(), F_OK) == -1) {
    return false;
  }

  return true;
}

static error open_pcm(FILE** fp, const std::string& filename) {
  if (!file_exist(filename)) {
    return error("File does not exist: " + filename, ERR::FILE_DOES_NOT_EXIST);
  }
  char command[1000];
  //opens interleaved pcm data stream
  snprintf(command, sizeof(command), "ffmpeg -i \"%s\" -f s16le -ar %d -ac 2 -v quiet -", filename.data(), gApp.audio->get_sample_rate());
  *fp = popen(command, "r");
  if (fp == nullptr) {
    printf(" %s\n", filename.data());
    return error("failed to open stream for " + filename, ERR::FAILED_TO_OPEN_FILE);
  }

  return noerror;
}

static error read_stem_from(FILE* fp, finyl_cstem& stem) {
  while (true) {
    finyl_sample* chunk = new finyl_sample[CHUNK_SIZE];
    size_t read = fread(chunk, sizeof(finyl_sample), CHUNK_SIZE, fp);
    stem.push_chunk(chunk);
    stem.add_ssize(read);
    if (read < CHUNK_SIZE) {
      break;
    }
    if (stem.chunks_size() == MAX_CHUNKS_SIZE) {
      printf("WARNING: file too large\n");
      break;
    }
  }
  
  return noerror;
}

Audio::Audio() {
  bqisoProcessor = std::make_unique<BiquadFullKillEQEffect>();
}

int Audio::wav_valid(char* addr) {
  if (addr == MAP_FAILED) {
    return -1;
  }
  
  short* nchannels = (short*)(addr+22);
  int* _sample_rate = (int*)(addr+24);
  short* bitdepth = (short*)(addr+34);
  
  if (*bitdepth != 16 || *nchannels != 2 || *_sample_rate != sample_rate) {
    return -1;
  }
  
  //find data chunk
  int data_offset = 36;
  char* data = (char*)(addr+data_offset);
  while (true) {
    if (data_offset > 10000) {
      return -1;
    }
    if (strncmp(data, "data", 4) == 0) break;
    data_offset++;
    data = (char*)(addr+data_offset);
  }
  
  return data_offset;
}

bool Audio::try_mmap(const std::string& file, std::unique_ptr<finyl_stem>& stem) {
  int fd = open(file.data(), O_RDONLY);
  struct stat sb;
  if (fstat(fd, &sb) == -1) {
    close(fd);
    return false;
  }
  char* addr = (char*)mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
  close(fd);
  
  int data_offset = wav_valid(addr);
  if (data_offset == -1) {
    if (munmap(addr, sb.st_size) != 0) {
      printf("failed to munmap\n");
    }
    return false;
  }

  int* ssize = (int*)(addr+data_offset+4);
  finyl_sample* data = (finyl_sample*)(addr+data_offset+8);

  auto mstem = std::make_unique<finyl_mstem>(data, sb.st_size, *ssize);
  stem.reset(mstem.release());
  return true;
}

error Audio::read_stem(const std::string& file, std::unique_ptr<finyl_stem>& stem) {
  //For wav file, its much faster to use mmap
  if (!file_exist(file)) {
    return error(ERR::NO_SUCH_FILE);
  }
  
  if (is_wav(file.data()) && try_mmap(file, stem)) {
    return noerror;
  }
  
  FILE* fp;
  auto err = open_pcm(&fp, file);
  if (err) return err;
  
  auto cstem = std::make_unique<finyl_cstem>();
  err = read_stem_from(fp, *cstem);
  if (err)  return err;

  stem.reset(cstem.release());
    
  return noerror;
}

//each file corresponding to each stem
error Audio::read_stems_from_files(const std::vector<std::string>& files, finyl_track& t) {
  if (files.size() > MAX_STEMS_SIZE) {
    return error("too many stems. MAX_STEMS_SIZE is " + std::to_string(MAX_STEMS_SIZE), ERR::TOO_MANY_STEMS);
  }
  
  std::vector<std::thread> threads;
  
  error err;
  for (size_t i = 0; i < files.size(); i++) {
    threads.push_back(std::thread([&, i]() {
      printf("reading %dth stem\n", (int)i);
      std::unique_ptr<finyl_stem> stem;
      auto _err = read_stem(files[i], stem);
      if (_err) {
        err = _err;
        return;
      }

      if (stem->ssize() == 0) {
        err = error("read_stem has read empty file?");
        return;
      }
      
      t.stems[i].reset(stem.release());
    }));
  }
  
  for (auto& t: threads) {
    t.join();
  }
  
  t.stems_size = files.size();
  
  return err;
}
//TODO: one file has many stems (eg. flac)
void read_stems_from_file(char* file);

void Audio::gain_effect(finyl_buffer& buf, double gain) {
  for (int i = 0; i<period_size_2;i=i+2) {
    buf[i] = gain * buf[i];
    buf[i+1] = gain * buf[i+1];
  }
}

//rubberband
//stretcher keeps storing "reqd" number of samples of samples in its internal buffer until output buffer can be fully prepared, leaving unused samples in the internal buffer.
//return new t.index. writes stretched samples to stem_buffer
int Audio::make_stem_buffer_stretch(finyl_buffer& stem_buffer, finyl_track& t, rb& stretcher, finyl_stem& stem, int index, std::mutex& mutex) {
  // Apply key shift by setting pitch scale on the stretcher
  double pitch_scale = t.get_pitch_scale();
  if (pitch_scale != stretcher.getPitchScale()) {
    std::lock_guard<std::mutex> lock(mutex);
    stretcher.setPitchScale(pitch_scale);
  }
  
  int available;
  while ((available = stretcher.available()) < period_size) {
    int reqd = int(ceil(double(period_size - available) / stretcher.getTimeRatio()));
    if (reqd > max_process_size) reqd = max_process_size;
    
    float inputLeft[reqd];
    float inputRight[reqd];
    for (int i = 0; i<reqd; i++) {
      index++;
      if (t.loop_active && t.loop_in != -1 && t.loop_out != -1 && index >= t.loop_out-1000) {
        index = t.loop_in + index - t.loop_out;
      }
      if (index >= t.get_refmsize()) {
        t.playing = false;
      }

      if (t.playing == false) {
        inputLeft[i] = 0;
        inputRight[i] = 0;
      } else {
        finyl_sample left, right;
        stem.get_samples(left, right, index);
        inputLeft[i] = float(left / 32768.0);
        inputRight[i] = float(right / 32768.0);
      }
    }
    
    float* inputs[2] = {inputLeft, inputRight};
    std::lock_guard<std::mutex> lock(mutex); //TODO: for rubberband2, not sure why this cant prevent seg fault
    stretcher.process(inputs, reqd, false);
  }
  float rubleft[period_size];
  float rubright[period_size];
  float* rubout[2] = {rubleft, rubright};
  
  stretcher.retrieve(rubout, period_size);

  for (int i = 0; i<period_size; i++) {
    int left = int(rubleft[i] * 32768);
    int right = int(rubright[i] * 32768);
    left = clip_sample(left);
    right = clip_sample(right);
      
    stem_buffer[i*2] = left;
    stem_buffer[i*2+1] = right;
  }

  return index;
}

//TODO slow, takes 500 microsecs
void Audio::make_stem_buffers_stretch(finyl_track& t, finyl_stem_buffers& stem_buffers) {
  std::vector<std::thread> threads;
  
  for (int i = 0; i<t.stems_size; i++) {
    threads.push_back(std::thread([&, i](){
      int newindex = make_stem_buffer_stretch(stem_buffers[i], t, *t.stretchers[i], *t.stems[i], t.indxs[i], t.mtxs[i]);
      if (!t.jump_lock) { //dont want to set index if jump happened during stretching
        t.indxs[i] = newindex;
      } else { //jump_lock was set to on during make_stem_buffer_stretch
      }
    }));
  }

  for (auto& th: threads) {th.join();}

}

//without time stretch
void Audio::make_stem_buffers(finyl_stem_buffers& stem_buffers, finyl_track& t) {
  for (int i = 0; i < period_size_2; i=i+2) {
    t.set_index(t.get_refindex() + t.get_speed());

    if (t.loop_active && t.loop_in != -1 && t.loop_out != -1 && t.get_refindex() >= (t.loop_out - 1000)) {
      t.set_index(t.loop_in + t.get_refindex() - t.loop_out);
    }
    
    if (t.get_refindex() >= t.get_refmsize()) {
      t.playing = false;
    }

    
    for (int c = 0; c<t.stems_size; c++) {
      auto& buf = stem_buffers[c];
      if (t.playing == false || t.get_refindex() < 0) {
        buf[i] = 0;
        buf[i+1] = 0;
      } else {
        auto& s = t.stems[c];
        s->get_samples(buf[i], buf[i+1], (int)t.get_refindex());
      }
    }
  }
}

finyl_sample clip_sample(int32_t s) {
  if (s > 32767) {
    s = 32767;
  } else if (s < -32768) {
    s = -32768;
  }
  return s;
}

void Audio::add_and_clip_two_buffers(finyl_buffer& dest, finyl_buffer& src1, finyl_buffer& src2) {
  for (int i = 0; i<period_size_2; i++) {
    int32_t sample = src1[i] + src2[i];
    dest[i] = clip_sample(sample);
  }
}

void Audio::signedshortToFloat(signed short* in, float* out) {
  for (int i = 0; i<period_size_2; i++) {
    out[i] = in[i] / 32768.0;
  }
}

void Audio::floatToSignedshort(float* in, signed short* out) {
  for (int i = 0; i<period_size_2; i++) {
    out[i] = in[i] * 32768.0;
  }
}

void Audio::eq_effect(finyl_buffer& buf, BiquadFullKillEQEffectGroupState* state) {
  float in[period_size_2];
  float out[period_size_2];

  signedshortToFloat(buf.data(), in);
  
  bqisoProcessor->process(state, in, out);

  floatToSignedshort(out, buf.data());
}

static void reset_stretchers(Deck& deck) {
  for (int i = 0; i<deck.pTrack->stems_size; i++) {
    deck.pTrack->stretchers[i]->reset();
  }
}

//NOTE: mute is implemented per buffer basis (simpler), contrary to per sample basis (better)
void Audio::mute_effect(finyl_buffer& buf, bool mute) {
  if (!mute) return;
  for (int i = 0; i<period_size_2;i=i+2) {
    buf[i] = 0;
    buf[i+1] = 0;
  }
}

void Audio::handle_deck(Deck& deck) {
  if (deck.pTrack == nullptr) return;
  if (deck.pTrack->jump_lock) {
    deck.pTrack->jump_lock = false;
    if (deck.master) {
      reset_stretchers(deck);
    }
  }
  if (deck.pTrack->playing) {
    if (deck.master) {
      make_stem_buffers_stretch(*deck.pTrack, deck.stem_buffers);
    } else {
      make_stem_buffers(deck.stem_buffers, *deck.pTrack);
    }
      
    gain_effect(deck.stem_buffers[0], deck.gain0);
    gain_effect(deck.stem_buffers[1], deck.gain1);
    mute_effect(deck.stem_buffers[0], deck.mute0);
    add_and_clip_two_buffers(deck.buffer, deck.stem_buffers[0], deck.stem_buffers[1]);
      
    eq_effect(deck.buffer, deck.bqisoState.get());
      
    gain_effect(deck.buffer, deck.gain);
  }
  
  deck.fftState->set_target(deck.buffer);
  deck.fftState->forward();
  //do stuff here!
  deck.fftState->inverse();
  
  delay(deck.buffer, *deck.delayState);

}

void Audio::handle_audio() {
  std::fill(buffer.begin(), buffer.end(), 0);
  std::fill(gApp.controller->adeck->buffer.begin(), gApp.controller->adeck->buffer.end(), 0);
  std::fill(gApp.controller->bdeck->buffer.begin(), gApp.controller->bdeck->buffer.end(), 0);
  std::fill(gApp.controller->adeck->stem_buffers[0].begin(), gApp.controller->adeck->stem_buffers[0].end(), 0);
  std::fill(gApp.controller->adeck->stem_buffers[1].begin(), gApp.controller->adeck->stem_buffers[1].end(), 0);
  std::fill(gApp.controller->bdeck->stem_buffers[0].begin(), gApp.controller->bdeck->stem_buffers[0].end(), 0);
  std::fill(gApp.controller->bdeck->stem_buffers[1].begin(), gApp.controller->bdeck->stem_buffers[1].end(), 0);
  
  auto t = std::thread([&](){
    handle_deck(*gApp.controller->adeck);
  });

  handle_deck(*gApp.controller->bdeck);
  
  t.join();
  add_and_clip_two_buffers(buffer, gApp.controller->adeck->buffer, gApp.controller->bdeck->buffer);
}

void Audio::setup_alsa_params() {
  snd_pcm_hw_params_t* hw_params;
  snd_pcm_hw_params_malloc(&hw_params);
  snd_pcm_hw_params_any(handle, hw_params);
  snd_pcm_hw_params_set_access(handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
  snd_pcm_hw_params_set_format(handle, hw_params, SND_PCM_FORMAT_S16_LE);
  snd_pcm_hw_params_set_rate(handle, hw_params, sample_rate, 0);
  snd_pcm_hw_params_set_channels(handle, hw_params, 2);
  snd_pcm_hw_params_set_period_size_near(handle, hw_params, &period_size, 0); //first set period
  snd_pcm_hw_params_set_buffer_size_near(handle, hw_params, &buffer_size); //then buffer. (NOTE: when device is set to default, buffer_size became period_size*3 although peirod_size*2 is requested)
  snd_pcm_hw_params(handle, hw_params);
  snd_pcm_get_params(handle, &buffer_size, &period_size);
  snd_pcm_hw_params_free(hw_params);
}

static void cleanup_alsa(snd_pcm_t* handle) {
  int err = snd_pcm_drain(handle);
  if (err < 0) {
    printf("snd_pcm_drain failed: %s\n", snd_strerror(err));
  }
  snd_pcm_close(handle);
  printf("alsa closed\n");
}

void Audio::on_period_size_change() {
  buffer.resize(period_size_2);
  gApp.controller->adeck.reset(new Deck(finyl_a));
  gApp.controller->bdeck.reset(new Deck(finyl_b));
}

void Audio::setup(const char* device) {
  setup_alsa(device);
  set_period_size(period_size);
  on_period_size_change();
}

void Audio::setup_alsa(const char* device) {
  int err;
  if ((err = snd_pcm_open(&handle, device, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
    printf("Playback open error!: %s\n", snd_strerror(err));
    exit(1);
  }
  setup_alsa_params();
  printf("buffer_size %ld, period_size %ld\n", buffer_size, period_size);
}

void Audio::set_period_size(int _period_size) {
  period_size = _period_size;
  period_size_2 = period_size * 2;
  buffer_size = period_size*2; //dont care if its 2 or 3
}

void Audio::run() {
  int err = 0;
  
  while (gApp.is_running()) {
    handle_audio();
    err = snd_pcm_writei(handle, buffer.data(), period_size);
    if (err == -EPIPE) {
      printf("Underrun occurred: %s\n", snd_strerror(err));
      snd_pcm_prepare(handle);
    } else if (err == -EAGAIN) {
      printf("eagain\n");
    } else if (err < 0) {
      printf("error %s\n", snd_strerror(err));
      gApp.stop_running();
      return;
    }
  }
  
  cleanup_alsa(handle);
  profile();
}

App::App() {
  audio = std::make_unique<Audio>();
  controller = std::make_unique<Controller>();
  interface = std::make_unique<Interface>();
}

void App::run() {
  auto t = std::thread([&](){
    audio->run();
  });
  controller->run();
  t.join();
}
