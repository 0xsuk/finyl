#ifndef FINYL_H
#define FINYL_H
#include <alsa/asoundlib.h>
#include "error.h"
#include <vector>
#include <string>
#include <array>

#ifndef MAX_CHUNKS_SIZE
#define MAX_CHUNKS_SIZE 32
#endif
#define CHUNK_SIZE 2097152 //(2048 * 1024);

#ifndef MAX_STEMS_SIZE //number of stems
#define MAX_STEMS_SIZE 5
#endif

using finyl_sample = signed short;
// using finyl_chunk = std::vector<finyl_sample>;
using finyl_stem = std::vector<finyl_sample>;
using finyl_buffer = std::vector<finyl_sample>;
using finyl_stem_buffers = std::array<finyl_buffer, MAX_STEMS_SIZE>;

extern double a0_gain;
extern double a1_gain;
extern double b0_gain;
extern double b1_gain;

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

struct finyl_track{
  finyl_track_meta meta;
  int length; //number of samples in a channel for each stem
  int stems_size;
  bool playing;
  double index; //index of pcm. left_sample = at(index*2), right_sample = at(index*2+1)
  double speed;
  bool loop_active;
  double loop_in; //index
  double loop_out; //index
  std::array<finyl_stem, MAX_STEMS_SIZE> stems;
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

extern bool finyl_running;

extern finyl_track* adeck; //pointer to track, is a d
extern finyl_track* bdeck;
extern finyl_track* cdeck;
extern finyl_track* ddeck;

bool file_exist(std::string_view file);

int finyl_get_quantized_beat_index(finyl_track& t, int index);
int finyl_get_quantized_time(finyl_track& t, int index);
double finyl_get_quantized_index(finyl_track& t, int index);
error finyl_read_stems_from_files(const std::vector<std::string>& files, finyl_track& t);
error read_stem(const std::string& file, finyl_stem& stem);
void finyl_setup_alsa(snd_pcm_t** handle, snd_pcm_uframes_t* buffer_size, snd_pcm_uframes_t* period_size);

void finyl_run(finyl_track* a, finyl_track* b, finyl_track* c, finyl_track* d, snd_pcm_t* handle, snd_pcm_uframes_t buffer_size, snd_pcm_uframes_t period_size);
#endif
