#ifndef FINYL_DEV_H
#define FINYL_DEV_H

#include <alsa/asoundlib.h>
#include "finyl.h"

/* void _print_track(track* t); */

/* void generateRandomString(char* badge, size_t size); */

/* void ncpy(char* dest, char* src, size_t size); */

/* char* read_file_malloc(char* filename); */

/* void init_track(track* t); */

/* void setup_alsa(snd_pcm_t** handle, snd_pcm_uframes_t* buffer_size, snd_pcm_uframes_t* period_size); */

/* int load_track(char* filename, track* t); */

void run(finyl_deck* a, finyl_deck* b, finyl_deck* c, finyl_deck* d, snd_pcm_t* handle, snd_pcm_uframes_t period_size);

#endif
