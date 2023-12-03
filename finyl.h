#ifndef FINYL_H
#define FINYL_H
#include <alsa/asoundlib.h>

#define chunks_size_max  32
#define chunk_size 2097152 //(2048 * 1024);
#define channels_size_max 8

typedef void (*finyl_process_callback)(unsigned long period_size, signed short buffer);

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
} finyl_track;

typedef enum {
  finyl_a,
  finyl_b,
  finyl_c,
  flnyl_d,
  finyl_all
} finyl_deck_enum;

typedef struct {
  finyl_track* t;
  finyl_deck_enum e;
  //next track here maybe
  //sync lock
} finyl_deck;

typedef struct {
  snd_pcm_t* handle;
  snd_pcm_uframes_t buffer_size;
  snd_pcm_uframes_t period_size;
} finyl_alsa;

void finyl_print_track(finyl_track* t);

void finyl_init_track_meta(finyl_track_meta* tm);

void finyl_init_track(finyl_track* t);

int finyl_read_channels_from_files(char** files, int channels_length, finyl_track* t);

int finyl_set_process_callback(finyl_process_callback cb, finyl_deck_enum e, int channel_index);

void finyl_setup_alsa(snd_pcm_t** handle, snd_pcm_uframes_t* buffer_size, snd_pcm_uframes_t* period_size);

void finyl_run(finyl_deck* a, finyl_deck* b, finyl_deck* c, finyl_deck* d, snd_pcm_t* handle, snd_pcm_uframes_t buffer_size, snd_pcm_uframes_t period_size);
#endif
