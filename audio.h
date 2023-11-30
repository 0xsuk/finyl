#ifndef AUDIO_H
#define AUDIO_H

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
  char name[300];
  chunks std_channel[maximum_chunks]; //array of chunks, chunk is array of pcm
  chunks voc_channel[maximum_chunks];
  chunks inst_channel[maximum_chunks]; //include drum when two_stems
  chunks drum_channel[maximum_chunks];
  int nchunks; //the number of chunks in a channel
  int length;
  double index;
  double speed;
  stem stem;
} track;

void init_track(track* t);

void setup_alsa(snd_pcm_t** handle, snd_pcm_uframes_t* buffer_size, snd_pcm_uframes_t* period_size);

int load_track(char* filename, track* t);

void run(track* t, snd_pcm_t* handle, snd_pcm_uframes_t period_size);

int get_playlists(char* usb);
#endif
