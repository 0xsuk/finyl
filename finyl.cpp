#include "finyl.h"
#include "error.h"
#include <thread>
#include <alsa/asoundlib.h>
#include <math.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <string_view>
#include <memory>

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

snd_pcm_uframes_t period_size;
snd_pcm_uframes_t period_size_2;

finyl_track_meta::finyl_track_meta(): id(0),
                                      bpm(0),
                                      musickeyid(0),
                                      filesize(0) {};

finyl_track::finyl_track(): meta(),
                            length(0),
                            stems_size(0),
                            playing(false),
                            index(0),
                            speed(1),
                            loop_active(false),
                            loop_in(-1),
                            loop_out(-1) {};


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

static error read_stem_from(FILE* fp, finyl_stem& stem) {
  int count = 20971520; //40mb*;
  int max = 4; //160mb;
  for (int i = 1; i<=max; i++) {
    int read_acc = count*(i-1);
    stem.resize(read_acc + count);
    size_t read = fread(&stem[read_acc], sizeof(finyl_sample), count, fp);
    if (read < count) { // finish
      stem.resize(read_acc + read);
      stem.shrink_to_fit();
      break;
    }
    if (i == max && read >= count) { //overflow, trim
      printf("WARNING: file too large, trimmed\n");
      break;
    }
  }
  
  return noerror;
}

error read_stem(const std::string& file, finyl_stem& stem) {
  FILE* fp;
  auto err = open_pcm(&fp, file);
  if (err) return err;
  
  err = read_stem_from(fp, stem);
  if (err) return err;
  
  return noerror;
}

//each file corresponding to each stem
error finyl_read_stems_from_files(const std::vector<std::string>& files, finyl_track& t) {
  if (files.size() > MAX_STEMS_SIZE) {
    return error("too many stems. MAX_STEMS_SIZE is " + std::to_string(MAX_STEMS_SIZE), ERR::TOO_MANY_STEMS);
  }
  
  std::vector<std::thread> threads;
  
  error err;
  int length = 0;
  for (size_t i = 0; i < files.size(); i++) {
    threads.push_back(std::thread([i, &files, &t, &err, &length]() {
      printf("reading %dth stem\n", (int)i);
      finyl_stem stem;
      auto _err = read_stem(files[i], stem);
      if (_err) {
        err = _err;
        return;
      }

      if (stem.size() == 0) {
        err = error("read_stem has read empty file?");
        return;
      }
      else if (length == 0) length = stem.size(); //initialize length
      else if (length != stem.size())  {
        err = error(ERR::STEMS_DIFFERENT_SIZE);
        return;
      }
      t.stems[i] = std::move(stem);
    }));
  }
  
  for (auto& t: threads) {
    t.join();
  }
  
  t.stems_size = files.size();
  t.length = length;
  
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

static void make_stem_buffers(finyl_stem_buffers& stem_buffers, finyl_track& t) {
  for (int i = 0; i < period_size_2; i=i+2) {
    t.index += t.speed;

    if (t.loop_active && t.loop_in != -1 && t.loop_out != -1 && t.index >= (t.loop_out - 1000)) {
      t.index = t.loop_in + t.index - t.loop_out;
    }
    
    if (t.index >= t.length) {
      t.playing = false;
    }

    for (int c = 0; c<t.stems_size; c++) {
      auto& buf = stem_buffers[c];
      int stereo_index = ((int)t.index) * 2;
      buf[i] = t.stems[c][stereo_index];
      buf[i+1] = t.stems[c][stereo_index+1];
    }
  }
}

static finyl_sample clip_sample(int32_t s) {
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

static void finyl_handle() {
  
  std::fill(buffer.begin(), buffer.end(), 0);
  std::fill(abuffer.begin(), abuffer.end(), 0);
  std::fill(bbuffer.begin(), bbuffer.end(), 0);
  
  if (adeck != nullptr && adeck->playing) {
    make_stem_buffers(a_stem_buffers, *adeck);
    gain_filter(a_stem_buffers[0], a0_gain);
    gain_filter(a_stem_buffers[1], a1_gain);
    add_and_clip_two_buffers(abuffer, a_stem_buffers[0], a_stem_buffers[1]);
  }
  
  if (bdeck != nullptr && bdeck->playing) {
    make_stem_buffers(b_stem_buffers, *bdeck);
    gain_filter(b_stem_buffers[0], b0_gain);
    gain_filter(b_stem_buffers[1], b1_gain);
    add_and_clip_two_buffers(bbuffer, b_stem_buffers[0], b_stem_buffers[1]);
  }
  
  add_and_clip_two_buffers(buffer, abuffer, bbuffer);
}

/* char* device = "hw:CARD=PCH,DEV=0";            /\* playback device *\/ */
char* device = "default";

static void setup_alsa_params(snd_pcm_t* handle, snd_pcm_uframes_t* buffer_size, snd_pcm_uframes_t* period_size) {
  snd_pcm_hw_params_t* hw_params;
  snd_pcm_hw_params_malloc(&hw_params);
  snd_pcm_hw_params_any(handle, hw_params);
  snd_pcm_hw_params_set_access(handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
  snd_pcm_hw_params_set_format(handle, hw_params, SND_PCM_FORMAT_S16_LE);
  snd_pcm_hw_params_set_rate(handle, hw_params, 44100, 0);
  snd_pcm_hw_params_set_channels(handle, hw_params, 2);
  snd_pcm_hw_params_set_buffer_size_near(handle, hw_params, buffer_size);
  snd_pcm_hw_params_set_period_size_near(handle, hw_params, period_size, 0);
  snd_pcm_hw_params(handle, hw_params);
  snd_pcm_hw_params_free(hw_params);
  snd_pcm_get_params(handle, buffer_size, period_size);
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
  if ((err = snd_pcm_open(handle, device, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
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

void finyl_run(finyl_track* a, finyl_track* b, finyl_track* c, finyl_track* d, snd_pcm_t* handle, snd_pcm_uframes_t buffer_size, snd_pcm_uframes_t _period_size) {
  int err = 0;
  period_size = _period_size;
  period_size_2 = 2*period_size;
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
}

