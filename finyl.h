#ifndef FINYL_H
#define FINYL_H

#include <alsa/asoundlib.h>

#define maximum_chunks  32
#define chunk_size 2097152 //(2048 * 1024);

typedef signed short sample;
typedef sample* chunks;

typedef enum {
  no_stem = 0, //normal track
  two_stems, //vocal and inst track
  three_stems, //vocal and inst and drum
} stem;

typedef struct {
  int id;
  int tempo;
  int musickeyid;
  int filesize;
  char title[300];
  char filepath[1000];
  char filename[300];
} track_meta;

typedef struct {
  chunks std_channel[maximum_chunks]; //array of chunks, chunk is array of pcm
  chunks voc_channel[maximum_chunks];
  chunks inst_channel[maximum_chunks]; //include drum when two_stems
  chunks drum_channel[maximum_chunks];
  track_meta meta;
  int nchunks; //the number of chunks in a channel
  int length;
  double index;
  double speed;
  stem stem;
} track;


void generateRandomString(char* badge, size_t size);

void ncpy(char* dest, char* src, size_t size);

char* read_file_malloc(char* filename);

void init_track(track* t);

void setup_alsa(snd_pcm_t** handle, snd_pcm_uframes_t* buffer_size, snd_pcm_uframes_t* period_size);

int load_track(char* filename, track* t);

void run(track* t, snd_pcm_t* handle, snd_pcm_uframes_t period_size);


#endif
