#ifndef FINYL_H
#define FINYL_H
#include <alsa/asoundlib.h>
#include "stdbool.h"

#define chunks_size_max  32
#define chunk_size 2097152 //(2048 * 1024);
#define channels_size_max 8

typedef signed short finyl_sample;
typedef finyl_sample* finyl_chunk; //chunk_size array of sample
typedef finyl_chunk* finyl_channel; //32 length array of pointer to chunk
typedef finyl_sample* finyl_multi_buffer;

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
  finyl_channel channels[8];
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
  finyl_a0,
  finyl_a1,
  finyl_a2,
  finyl_a3,
  finyl_a4,
  finyl_a5,
  finyl_a6,
  finyl_a7,
  finyl_b,
  finyl_b0,
  finyl_b1,
  finyl_b2,
  finyl_b3,
  finyl_b4,
  finyl_b5,
  finyl_b6,
  finyl_b7,
  finyl_all
} finyl_callback_target;

/* typedef struct { */
  /* snd_pcm_t* handle; */
  /* snd_pcm_uframes_t buffer_size; */
  /* snd_pcm_uframes_t period_size; */
/* } finyl_alsa; */

typedef void (*finyl_process_callback)(unsigned long period_size, finyl_sample* buffer, finyl_track* t);

finyl_sample finyl_get_sample(finyl_track* t, finyl_channel c);

void finyl_print_track(finyl_track* t);

void finyl_init_track_meta(finyl_track_meta* tm);

void finyl_init_track(finyl_track* t);

int finyl_read_channels_from_files(char** files, int channels_length, finyl_track* t);

int finyl_set_process_callback(finyl_process_callback cb, finyl_callback_target ct);

void finyl_handle_play(unsigned long period_size, finyl_sample* buffer, finyl_track* t);

void finyl_handle_master(unsigned long period_size, finyl_sample* buf, finyl_track* t);

void finyl_setup_alsa(snd_pcm_t** handle, snd_pcm_uframes_t* buffer_size, snd_pcm_uframes_t* period_size);

void finyl_run(finyl_track* a, finyl_track* b, finyl_track* c, finyl_track* d, snd_pcm_t* handle, snd_pcm_uframes_t buffer_size, snd_pcm_uframes_t period_size);
#endif
