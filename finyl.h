#ifndef FINYL_H
#define FINYL_H
#include <alsa/asoundlib.h>
#include "stdbool.h"
#include <vector>
#include <string>
#include <array>

#ifndef MAX_CHUNKS_SIZE
#define MAX_CHUNKS_SIZE 32
#endif
#define CHUNK_SIZE 2097152 //(2048 * 1024);

#ifndef MAX_CHANNELS_SIZE //number of stems
#define MAX_CHANNELS_SIZE 5
#endif

using finyl_sample = signed short;
using finyl_chunk = std::vector<finyl_sample>;
using finyl_channel = std::vector<finyl_chunk>;
using finyl_buffer = std::vector<finyl_sample>;
using finyl_channel_buffers = std::array<finyl_buffer, MAX_CHANNELS_SIZE>;

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
  int id = 0;
  int bpm = 0;
  int musickeyid = 0;
  int filesize = 0;
  std::string title;
  std::string filename; //caution: filename is compelte. filepath is sometimes incomplete. just use filepath
  std::string filepath;
  std::vector<std::string> channel_filepaths;
}; //used in track listing in playlist

struct finyl_track{
  finyl_track_meta meta;
  // int chunks_size; //the number of chunks in a channel
  int length = 0; //number of samples per channel
  bool playing = false;
  double index = 0;
  double speed = 0;
  bool loop_active = false;
  double loop_in = -1; //index
  double loop_out = -1; //index
  std::vector<finyl_channel> channels;
  std::vector<finyl_cue> cues;
  std::vector<finyl_beat> beats;
};

enum finyl_track_target{
  finyl_a,
  finyl_b,
  finyl_c,
  finyl_d,
};

enum finyl_channel_target{
  finyl_0,
  finyl_1,
  finyl_2,
  finyl_3,
  finyl_4,
  finyl_5,
  finyl_6,
  finyl_7,
};

extern bool finyl_running;

extern finyl_track* adeck; //pointer to track, is a d
extern finyl_track* bdeck;
extern finyl_track* cdeck;
extern finyl_track* ddeck;

bool file_exist(std::string file);

finyl_sample finyl_get_sample(finyl_track& t, finyl_channel& c);
finyl_sample finyl_get_sample1(finyl_channel& c, int position);

int finyl_get_quantized_beat_index(finyl_track& t, int index);
int finyl_get_quantized_time(finyl_track& t, int index);
double finyl_get_quantized_index(finyl_track& t, int index);
int finyl_read_channels_from_files(std::vector<std::string>& files, finyl_track& t);

void finyl_setup_alsa(snd_pcm_t** handle, snd_pcm_uframes_t* buffer_size, snd_pcm_uframes_t* period_size);

void finyl_run(finyl_track* a, finyl_track* b, finyl_track* c, finyl_track* d, snd_pcm_t* handle, snd_pcm_uframes_t buffer_size, snd_pcm_uframes_t period_size);
#endif
