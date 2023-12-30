#ifndef FINYL_H
#define FINYL_H
#include <alsa/asoundlib.h>
#include "stdbool.h"
#include <vector>

#ifndef MAX_CHUNKS_SIZE
#define MAX_CHUNKS_SIZE 32
#endif
#define CHUNK_SIZE 2097152 //(2048 * 1024);

#ifndef MAX_CHANNELS_SIZE //number of stems
#define MAX_CHANNELS_SIZE 5
#endif

typedef signed short finyl_sample;
typedef finyl_sample* finyl_chunk; //chunk_size array of sample
typedef finyl_chunk* finyl_channel; //32 length array of chunk


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
  char* name;
};

struct finyl_track_meta {
  int id;
  int bpm;
  int musickeyid;
  int filesize;
  char* title;
  char* filename; //caution: filename is compelte. filepath is sometimes incomplete. just use filepath
  char* filepath;
  char* channel_filepaths[MAX_CHANNELS_SIZE];
  int channels_size; //number of stems available
}; //used in track listing in playlist

struct finyl_track{
  finyl_channel channels[MAX_CHANNELS_SIZE];
  int channels_size; //number of stems
  finyl_track_meta meta;
  int chunks_size; //the number of chunks in a channel
  int length;
  bool playing;
  double index;
  double speed;
  double loop_in; //index
  double loop_out; //index
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

bool file_exist(char* file);
void finyl_free_track_metas(finyl_track_meta* tms, int size);
void finyl_free_in_track_meta(finyl_track_meta* tm);

void finyl_free_in_track(finyl_track* t);
void finyl_free_track(finyl_track* t);

finyl_sample finyl_get_sample(finyl_track* t, finyl_channel c);
finyl_sample finyl_get_sample1(finyl_channel c, int position);

void finyl_init_track(finyl_track* t);
void finyl_init_track_meta(finyl_track_meta* tm);

int finyl_get_quantized_beat_index(finyl_track* t, int index);
int finyl_get_quantized_time(finyl_track* t, int index);

int finyl_read_channels_from_files(char** files, int channels_length, finyl_track* t);

void finyl_track_callback_play(unsigned long period_size, finyl_sample* buf, finyl_track* t);

void finyl_setup_alsa(snd_pcm_t** handle, snd_pcm_uframes_t* buffer_size, snd_pcm_uframes_t* period_size);

void finyl_run(finyl_track* a, finyl_track* b, finyl_track* c, finyl_track* d, snd_pcm_t* handle, snd_pcm_uframes_t buffer_size, snd_pcm_uframes_t period_size);
#endif
