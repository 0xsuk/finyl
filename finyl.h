#ifndef FINYL_H
#define FINYL_H
#include <alsa/asoundlib.h>
#include "error.h"
#include <vector>
#include <memory>
#include <sys/mman.h>
#include <string>
#include <atomic>
#include "RubberBandStretcher.h"

#define CHUNK_SIZE 2097152 //4mb
#define MAX_CHUNKS_SIZE 64

#ifndef MAX_STEMS_SIZE //number of stems
#define MAX_STEMS_SIZE 5
#endif

using finyl_sample = signed short; // because wav file is signed short, and can be mmap-ed if finyl_sample is signed short, although for other data types, interpreting data as signed short after mmap is possible
//signed short (16-bit) is enough because the sound quality of stem is not so good that more than 16-bit representation does not worth the cost
using finyl_buffer = std::vector<finyl_sample>;
using finyl_stem_buffers = finyl_buffer[MAX_STEMS_SIZE];
using rb = RubberBand::RubberBandStretcher;

enum class CUE {
  ACTIVE_MEMORY_CUE,
  MEMORY_CUE,
  HOTCUE,
};

struct finyl_cue{
  CUE type;
  double time; //in millisec
};

struct finyl_beat{
  int time;
  int number; //1 to 4
};

struct finyl_playlist{
  int id;
  std::string name;
};

struct finyl_track_meta {
  int id;
  int bpm;
  int musickeyid;
  int filesize;
  std::string title;
  std::string filename; //caution: filename is compelte. filepath is sometimes incomplete. just use filepath
  std::string filepath;
  std::vector<std::string> stem_filepaths;

  finyl_track_meta();
}; //used in track listing in playlist

struct finyl_stem {
protected:
  size_t msize_ = 0;
  size_t ssize_ = 0;
public:
  virtual finyl_sample operator[](int sindex) = 0;
  virtual void get_samples(finyl_sample& left, finyl_sample& right, int mindex) = 0;
  size_t msize() {
    return msize_;
  }
  size_t ssize() {
    return ssize_;
  }
  virtual ~finyl_stem() = default;
};

//mmapped stem
struct finyl_mstem: public finyl_stem {
private:
  finyl_sample* data;
  int file_size;
  
public:
  finyl_mstem(finyl_sample* _data, int _filesize, size_t data_size) {
    data = _data;
    file_size = _filesize;
    ssize_ = data_size/2; //short
    msize_ = ssize_/2;
  }
  finyl_mstem(finyl_mstem&&) = default;

  void get_samples(finyl_sample& left, finyl_sample& right, int mindex) {
    if (mindex < 0) {
      left = 0;
      right = 0;
      return;
    }
    int sindex = mindex*2;
    left = data[sindex];
    right = data[sindex+1];
  }
  
  finyl_sample operator[](int sindex) {
    return data[sindex];
  }

  ~finyl_mstem() {
    if (data != nullptr) {
      munmap(data, file_size);
      data = nullptr;
    }
  }
};

struct finyl_cstem: public finyl_stem {
private:
  finyl_sample* data[MAX_CHUNKS_SIZE];
  size_t chunks_size_ = 0;
  
public:
  finyl_cstem() = default;
  finyl_cstem(finyl_cstem&&) = default;
  size_t chunks_size() {
    return chunks_size_;
  }
  
  void add_ssize(size_t _add) {
    ssize_ += _add;
    msize_ = ssize_/2;
  }
  
  void push_chunk(finyl_sample* chunk) {
    data[chunks_size_] = chunk;
    chunks_size_++;
  }
  
  void get_samples(finyl_sample& left, finyl_sample& right, int mindex) {
    if (mindex < 0) {
      left = 0;
      right = 0;
      return;
    }
    int sindex = mindex*2;
    int ichunk = sindex / CHUNK_SIZE;
    int isample = sindex - (CHUNK_SIZE * ichunk);
    left = data[ichunk][isample];
    right = data[ichunk][isample+1];
  }
  
  finyl_sample operator[](int sindex) {
    int ichunk = sindex / CHUNK_SIZE;
    int isample = sindex - (CHUNK_SIZE * ichunk);
    return data[ichunk][isample];
  }

  ~finyl_cstem() {
    for (int ichunk=0; ichunk<chunks_size_; ichunk++) {
      delete[] data[ichunk];
      data[ichunk] = nullptr;
    }
  }
};

struct finyl_track{
  finyl_track_meta meta;
  int stems_size;
  std::atomic<bool> jump_lock;
  std::atomic<bool> playing;
  std::atomic<bool> loop_active;
  std::atomic<double> loop_in; //index
  std::atomic<double> loop_out; //index
  std::mutex mtxs[MAX_STEMS_SIZE]; //prevent setTimeRatio happen during rubberband process()
  std::atomic<double> indxs[MAX_STEMS_SIZE]; //index for stem, used by rubberband stretcher
  std::unique_ptr<rb> stretchers[MAX_STEMS_SIZE];
  std::unique_ptr<finyl_stem> stems[MAX_STEMS_SIZE];
  std::vector<finyl_cue> cues;
  std::vector<finyl_beat> beats;

  finyl_track();
  int get_refmsize() {
    return stems[0]->msize();
  }
  
  double get_refindex() { //this index just references the first stem's index
    return indxs[0];
  }
  void set_index(double index) { //sets all stems
    for (int s = 0; s<stems_size; s++) {
      indxs[s] = index;
    }
  }
  void set_speed(double speed) {
    double timeratio = 1.0/speed;
    set_timeratio(timeratio);
  }
  double get_speed() {
    return 1.0/get_reftimeratio();
  }
  double get_reftimeratio() {
    return stretchers[0]->getTimeRatio();
  }
  void set_timeratio(double ratio) { //sets for all stems
    if (ratio <= 0) {
      return;
    }
    for (int i = 0; i<stems_size; i++) {
      std::lock_guard<std::mutex> lock(mtxs[i]);
      stretchers[i]->setTimeRatio(ratio);
    }
  }
  
};

enum finyl_deck_type{
  finyl_a,
  finyl_b,
  finyl_c,
  finyl_d,
};

struct Delay;
class BiquadFullKillEQEffectGroupState;
class BiquadFullKillEQEffect;
struct ActionState;
class Deck { //Should be initialized after audio is ready
public:
  finyl_deck_type type;
  finyl_track* pTrack;
  finyl_buffer buffer;
  finyl_stem_buffers stem_buffers;
  ActionState* action_state;
  double gain;
  double gain0;
  double gain1;
  bool mute0;
  double filter1;
  bool quantize;
  bool master;

  //TODO: these effects is sensitive to sample_rate. Effects reset function should be hooked to on_change_sample_rate
  std::unique_ptr<Delay> delay;
  std::unique_ptr<BiquadFullKillEQEffectGroupState> bqisoState;

  Deck(finyl_deck_type _type);
};


class Controller;
class Interface;
class App;

class Audio {
public:
  Audio();
  void run();

  void set_period_size(int _period_size);
  
  snd_pcm_uframes_t get_period_size() {return period_size;}
  snd_pcm_uframes_t get_period_size_2() {return period_size_2;}
  int get_sample_rate() {return sample_rate;}
  
  void setup_alsa_params();
  void setup_alsa(const char* device);
  
  error read_stems_from_files(const std::vector<std::string>& files, finyl_track& t);
  int max_process_size = 2048;

private:
  void on_period_size_change();
  int make_stem_buffer_stretch(finyl_buffer& stem_buffer, finyl_track& t, rb& stretcher, finyl_stem& stem, int index, std::mutex& mutex);
  int wav_valid(char* addr);
  bool try_mmap(const std::string& file, std::unique_ptr<finyl_stem>& stem);
  error read_stem(const std::string& file, std::unique_ptr<finyl_stem>& stem);
  void gain_effect(finyl_buffer& buf, double gain);
  void make_stem_buffers_stretch(finyl_track& t, finyl_stem_buffers& stem_buffers);
  void make_stem_buffers(finyl_stem_buffers& stem_buffers, finyl_track& t);
  void add_and_clip_two_buffers(finyl_buffer& dest, finyl_buffer& src1, finyl_buffer& src2);
  void eq_effect(finyl_buffer& buf, BiquadFullKillEQEffectGroupState* state);
  void signedshortToFloat(signed short* in, float* out);
  void floatToSignedshort(float* in, signed short* out);
  void mute_effect(finyl_buffer& buf, bool mute);
  void handle_deck(Deck& deck);
  void handle_audio();
  
  snd_pcm_uframes_t buffer_size = 256;
  snd_pcm_uframes_t period_size = 128;
  snd_pcm_uframes_t period_size_2 = 256;
  int sample_rate = 44100;
  finyl_buffer buffer;
  snd_pcm_t* handle;

  std::unique_ptr<BiquadFullKillEQEffect> bqisoProcessor;
};

class App {
public:
  App();
  void stop_running() {
    running = false;
  }
  
  bool is_running() {
    return running;
  }

  void run();

  std::shared_ptr<Audio> audio;
  std::shared_ptr<Controller> controller;
  std::shared_ptr<Interface> interface;
private:
  std::atomic<bool> running = true;
};

finyl_sample clip_sample(int32_t s);
bool file_exist(std::string_view file);
int finyl_get_quantized_beat_index(finyl_track& t, int index);
int finyl_get_quantized_time(finyl_track& t, int index);
double finyl_get_quantized_index(finyl_track& t, int index);

extern App gApp;
#endif
