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

std::string device;
int fps = 30;
snd_pcm_uframes_t period_size;
snd_pcm_uframes_t period_size_2;

bool finyl_running = true;

finyl_track* adeck; //pointer to track, is a d
finyl_track* bdeck;
finyl_track* cdeck;
finyl_track* ddeck;

finyl_buffer buffer;
finyl_buffer abuffer;
finyl_stem_buffers a_stem_buffers;
finyl_buffer bbuffer;
finyl_stem_buffers b_stem_buffers;

double a_gain = 1.0;
double a0_gain = 0.0;
double a1_gain = 0.0;
double b_gain = 1.0;
double b0_gain = 0.0;
double b1_gain = 0.0;
double a0_filter = 0.2;
double a1_filter = 1.0;
double b0_filter = 0.2;
double b1_filter = 1.0;
Delay a_delay{.wetmix=0.5, .drymix=1.0, .feedback=0.5};
Delay b_delay{.wetmix=0.5, .drymix=1.0, .feedback=0.5};
rb a0_stretcher(44100, 2, rb::OptionProcessRealTime, 1.0);
rb a1_stretcher(44100, 2, rb::OptionProcessRealTime, 1.0);
rb* a_stretchers[2] = {&a0_stretcher, &a1_stretcher};
rb b0_stretcher(44100, 2, rb::OptionProcessRealTime, 1.0);
rb b1_stretcher(44100, 2, rb::OptionProcessRealTime, 1.0);
rb* b_stretchers[2] = {&b0_stretcher, &b1_stretcher};
bool a_master = true;
bool b_master = true;

finyl_track_meta::finyl_track_meta(): id(0),
                                      bpm(0),
                                      musickeyid(0),
                                      filesize(0) {};

finyl_track::finyl_track(): meta(),
                            msize(0),
                            stems_size(0),
                            playing(false),
                            speed(1),
                            loop_active(false),
                            loop_in(-1),
                            loop_out(-1) {
  std::fill(indxs.begin(), indxs.end(), 0);
};




int finyl_get_quantized_beat_index(finyl_track& t, int index) {
  if (t.beats.size() <= 0) {
    return -1;
  }
  
  //44100 samples = 1sec
  //index samples = 1 / 44100 * index sec
  //= 1 / 44100 * index * 1000 millisec
  double nowtime = 1000.0 / 44100.0 * index;

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
  double v =  44.1 * finyl_get_quantized_time(t, index);

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
  snprintf(command, sizeof(command), "ffmpeg -i \"%s\" -f s16le -ar 44100 -ac 2 -v quiet -", filename.data());
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

static int wav_valid(char* addr) {
  if (addr == MAP_FAILED) {
    return -1;
  }
  
  short* nchannels = (short*)(addr+22);
  int* sample_rate = (int*)(addr+24);
  short* bitdepth = (short*)(addr+34);
  
  if (*bitdepth != 16 || *nchannels != 2 || *sample_rate != 44100) {
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

static bool try_mmap(const std::string& file, std::unique_ptr<finyl_stem>& stem) {
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

error read_stem(const std::string& file, std::unique_ptr<finyl_stem>& stem) {
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
error finyl_read_stems_from_files(const std::vector<std::string>& files, finyl_track& t) {
  if (files.size() > MAX_STEMS_SIZE) {
    return error("too many stems. MAX_STEMS_SIZE is " + std::to_string(MAX_STEMS_SIZE), ERR::TOO_MANY_STEMS);
  }
  
  std::vector<std::thread> threads;
  
  error err;
  int msize = 0;
  for (size_t i = 0; i < files.size(); i++) {
    threads.push_back(std::thread([i, &files, &t, &err, &msize]() {
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
      else if (msize == 0) msize = stem->msize(); //initialize length
      else if (msize != stem->msize())  {
        err = error(ERR::STEMS_DIFFERENT_SIZE);
        return;
      }
      t.stems[i].reset(stem.release());
    }));
  }
  
  for (auto& t: threads) {
    t.join();
  }
  
  t.stems_size = files.size();
  t.msize = msize;
  
  return err;
}
//TODO: one file has many stems (eg. flac)
void read_stems_from_file(char* file);

static void gain_filter(finyl_buffer& buf, double gain) {
  for (int i = 0; i<period_size_2;i=i+2) {
    buf[i] = gain * buf[i];
    buf[i+1] = gain * buf[i+1];
  }
}

//return new t.index. writes stretched samples to stem_buffer
static int make_stem_buffer_stretch(finyl_buffer& stem_buffer, finyl_track& t, rb& stretcher, finyl_stem& stem, int index) {
  int available;
  while ((available = stretcher.available()) < period_size) {
    int reqd = int(ceil(double(period_size - available) / stretcher.getTimeRatio()));
    reqd = std::max(reqd, int(stretcher.getSamplesRequired()));
    reqd = std::min(reqd, 16384);
    if (reqd == 0) reqd = 1;
    
    float inputLeft[reqd];
    float inputRight[reqd];
    for (int i = 0; i<reqd; i++) {
      index++;
      if (t.loop_active && t.loop_in != -1 && t.loop_out != -1 && index >= t.loop_out) {
        index = t.loop_in + index - t.loop_out;
      }
      if (index >= t.msize) {
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

//rubberband
static void make_stem_buffers_stretch(finyl_track& t, finyl_stem_buffers& stem_buffers, rb** stretchers) {
  std::vector<std::thread> threads;
  
  for (int i = 0; i<t.stems_size; i++) {
    threads.push_back(std::thread([&, i](){
      t.indxs[i] = make_stem_buffer_stretch(stem_buffers[i], t, *stretchers[i], *t.stems[i], t.indxs[i]);
    }));
  }

  for (auto& t: threads) {t.join();}
}

//without time stretch
static void make_stem_buffers(finyl_stem_buffers& stem_buffers, finyl_track& t) {
  for (int i = 0; i < period_size_2; i=i+2) {
    t.set_index(t.get_refindex() + t.speed);

    if (t.loop_active && t.loop_in != -1 && t.loop_out != -1 && t.get_refindex() >= (t.loop_out - 1000)) {
      t.set_index(t.loop_in + t.get_refindex() - t.loop_out);
    }
    
    if (t.get_refindex() >= t.msize) {
      t.playing = false;
    }

    for (int c = 0; c<t.stems_size; c++) {
      auto& buf = stem_buffers[c];
      if (t.playing == false) {
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

static void add_and_clip_two_buffers(finyl_buffer& dest, finyl_buffer& src1, finyl_buffer& src2) {
  for (int i = 0; i<period_size_2; i++) {
    int32_t sample = src1[i] + src2[i];
    dest[i] = clip_sample(sample);
  }
}

static void finyl_handle() {
  
  std::fill(buffer.begin(), buffer.end(), 0);
  std::fill(abuffer.begin(), abuffer.end(), 0);
  std::fill(bbuffer.begin(), bbuffer.end(), 0);
  
  if (adeck != nullptr && adeck->playing) {
    if (a_master) {
      make_stem_buffers_stretch(*adeck, a_stem_buffers, a_stretchers);
    } else {
      make_stem_buffers(a_stem_buffers, *adeck);
    }
    gain_filter(a_stem_buffers[0], a0_gain);
    gain_filter(a_stem_buffers[1], a1_gain);
    add_and_clip_two_buffers(abuffer, a_stem_buffers[0], a_stem_buffers[1]);
    gain_filter(abuffer, a_gain);
  }
  delay(abuffer, a_delay);
  if (bdeck != nullptr && bdeck->playing) {
    make_stem_buffers_stretch(*bdeck, b_stem_buffers, b_stretchers);
    gain_filter(b_stem_buffers[0], b0_gain);
    gain_filter(b_stem_buffers[1], b1_gain);
    add_and_clip_two_buffers(bbuffer, b_stem_buffers[0], b_stem_buffers[1]);
    gain_filter(bbuffer, b_gain);
  }
  delay(bbuffer, b_delay);
  
  add_and_clip_two_buffers(buffer, abuffer, bbuffer);
}

static void setup_alsa_params(snd_pcm_t* handle, snd_pcm_uframes_t* buffer_size, snd_pcm_uframes_t* period_size) {
  snd_pcm_hw_params_t* hw_params;
  snd_pcm_hw_params_malloc(&hw_params);
  snd_pcm_hw_params_any(handle, hw_params);
  snd_pcm_hw_params_set_access(handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
  snd_pcm_hw_params_set_format(handle, hw_params, SND_PCM_FORMAT_S16_LE);
  snd_pcm_hw_params_set_rate(handle, hw_params, 44100, 0);
  snd_pcm_hw_params_set_channels(handle, hw_params, 2);
  snd_pcm_hw_params_set_period_size_near(handle, hw_params, period_size, 0); //first set period
  snd_pcm_hw_params_set_buffer_size_near(handle, hw_params, buffer_size); //then buffer. (NOTE: when device is set to default, buffer_size became period_size*3 although peirod_size*2 is requested)
  snd_pcm_hw_params(handle, hw_params);
  snd_pcm_get_params(handle, buffer_size, period_size);
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

void finyl_setup_alsa(snd_pcm_t** handle, snd_pcm_uframes_t* buffer_size, snd_pcm_uframes_t* period_size) {
  int err;
  if ((err = snd_pcm_open(handle, device.data(), SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
    printf("Playback open error!: %s\n", snd_strerror(err));
    exit(1);
  }
  setup_alsa_params(*handle, buffer_size, period_size);
  printf("buffer_size %ld, period_size %ld\n", *buffer_size, *period_size);
}

static void resize_buffers() {
  buffer.resize(period_size_2);
  abuffer.resize(period_size_2);
  bbuffer.resize(period_size_2);
  
  for (size_t i = 0; i<MAX_STEMS_SIZE; i++) {
    a_stem_buffers[i].resize(period_size_2);
    b_stem_buffers[i].resize(period_size_2);
  }
}

static void init_decks(finyl_track* a, finyl_track* b, finyl_track* c, finyl_track* d) {
  adeck = a;
  bdeck = b;
  cdeck = c;
  ddeck = d;
}

void finyl_run(finyl_track* a, finyl_track* b, finyl_track* c, finyl_track* d, snd_pcm_t* handle) {
  int err = 0;
  init_decks(a, b, c, d);
  resize_buffers();
  
  while (finyl_running) {
    finyl_handle();
    
    err = snd_pcm_writei(handle, buffer.data(), period_size);
    if (err == -EPIPE) {
      printf("Underrun occurred: %s\n", snd_strerror(err));
      snd_pcm_prepare(handle);
    } else if (err == -EAGAIN) {
      printf("eagain\n");
    } else if (err < 0) {
      printf("error %s\n", snd_strerror(err));
      finyl_running = false;
      return;
    }
  }
  
  cleanup_alsa(handle);
  profile();
}
