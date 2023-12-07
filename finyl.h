#ifndef FINYL_H
#define FINYL_H
#include <alsa/asoundlib.h>
#include "stdbool.h"

#ifndef MAX_CHUNKS_SIZE
#define MAX_CHUNKS_SIZE 32
#endif
#define CHUNK_SIZE 2097152 //(2048 * 1024);

#ifndef MAX_CHANNELS_SIZE //number of stems
#define MAX_CHANNELS_SIZE 2
#endif

typedef signed short finyl_sample;
typedef finyl_sample* finyl_chunk; //chunk_size array of sample
typedef finyl_chunk* finyl_channel; //32 length array of chunk


extern double a0_gain;
extern double a1_gain;

typedef struct {
  int id;
  int tempo;
  int musickeyid;
  int filesize;
  char title[300];
  char filepath[1000];
  char filename[300];
} finyl_track_meta;

typedef struct {
  finyl_channel channels[MAX_CHANNELS_SIZE];
  finyl_track_meta meta;
  int nchunks; //the number of chunks in a channel
  int length;
  double index;
  double speed;
  int channels_size; //number of stems
  bool playing;
} finyl_track;

typedef enum {
  finyl_a,
  finyl_b,
  finyl_c,
  finyl_d,
} finyl_track_target;

typedef enum {
  finyl_0,
  finyl_1,
  finyl_2,
  finyl_3,
  finyl_4,
  finyl_5,
  finyl_6,
  finyl_7,
} finyl_channel_target;

extern finyl_track* adeck; //pointer to track, is a d
extern finyl_track* bdeck;
extern finyl_track* cdeck;
extern finyl_track* ddeck;

finyl_sample finyl_get_sample(finyl_track* t, finyl_channel c);

void finyl_print_track(finyl_track* t);

void finyl_init_track_meta(finyl_track_meta* tm);

void finyl_init_track(finyl_track* t);

int finyl_read_channels_from_files(char** files, int channels_length, finyl_track* t);

void finyl_track_callback_play(unsigned long period_size, finyl_sample* buf, finyl_track* t);

void finyl_setup_alsa(snd_pcm_t** handle, snd_pcm_uframes_t* buffer_size, snd_pcm_uframes_t* period_size);

void finyl_run(finyl_track* a, finyl_track* b, finyl_track* c, finyl_track* d, snd_pcm_t* handle, snd_pcm_uframes_t buffer_size, snd_pcm_uframes_t period_size);
#endif
