#ifndef FINYL_H
#define FINYL_H
#include <alsa/asoundlib.h>
#include "error.h"
#include <vector>
#include <memory>
#include <sys/mman.h>
#include <string>
#include <array>

#define CHUNK_SIZE 2097152 //4mb
#define MAX_CHUNKS_SIZE 64

#ifndef MAX_STEMS_SIZE //number of stems
#define MAX_STEMS_SIZE 5
#endif

using finyl_sample = signed short;
using finyl_buffer = std::vector<finyl_sample>;
using finyl_stem_buffers = std::array<finyl_buffer, MAX_STEMS_SIZE>;

struct finyl_cue{
  int type;
  int time;
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
  virtual finyl_sample operator[](size_t sindex) = 0;
  virtual void get_samples(finyl_sample& left, finyl_sample& right, size_t mindex) = 0;
  size_t msize() {
    return msize_;
  }
  size_t ssize() {
    return ssize_;
  }
  virtual ~finyl_stem() = default;
};

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

  void get_samples(finyl_sample& left, finyl_sample& right, size_t mindex) {
    int sindex = mindex*2;
    left = data[sindex];
    right = data[sindex+1];
  }
  
  finyl_sample operator[](size_t sindex) {
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
  
  void get_samples(finyl_sample& left, finyl_sample& right, size_t mindex) {
    size_t sindex = mindex*2;
    int ichunk = sindex / CHUNK_SIZE;
    int isample = sindex - (CHUNK_SIZE * ichunk);
    left = data[ichunk][isample];
    right = data[ichunk][isample+1];
  }
  
  finyl_sample operator[](size_t sindex) {
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
  int msize; //number of samples in a channel for each stem
  int stems_size;
  bool playing;
  double index; //index of pcm. left_sample = at(index*2), right_sample = at(index*2+1)
  double speed;
  bool loop_active;
  double loop_in; //index
  double loop_out; //index
  std::array<std::unique_ptr<finyl_stem>, MAX_STEMS_SIZE> stems;
  std::vector<finyl_cue> cues;
  std::vector<finyl_beat> beats;

  finyl_track();
};

enum finyl_track_target{
  finyl_a,
  finyl_b,
  finyl_c,
  finyl_d,
};

enum finyl_stem_target{
  finyl_0,
  finyl_1,
  finyl_2,
  finyl_3,
  finyl_4,
};

finyl_sample clip_sample(int32_t s);
bool file_exist(std::string_view file);
int finyl_get_quantized_beat_index(finyl_track& t, int index);
int finyl_get_quantized_time(finyl_track& t, int index);
double finyl_get_quantized_index(finyl_track& t, int index);
error finyl_read_stems_from_files(const std::vector<std::string>& files, finyl_track& t);
error read_stem(const std::string& file, std::unique_ptr<finyl_stem>& stem);
void finyl_setup_alsa(snd_pcm_t** handle, snd_pcm_uframes_t* buffer_size, snd_pcm_uframes_t* period_size);

void finyl_run(finyl_track* a, finyl_track* b, finyl_track* c, finyl_track* d, snd_pcm_t* handle);
#endif
