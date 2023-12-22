#ifndef INTERFACE_H
#define INTERFACE_H

#include "finyl.h"
#include <SDL2/SDL.h>

extern int amount;
extern bool render_adeck;
extern bool render_bdeck;

int get_window_size();

void renderWaveform(SDL_Renderer *renderer, finyl_sample* pcmData, int numSamples);

void add_track_to_free(finyl_track* t);

void free_tracks();

int interface(finyl_track* adeck, finyl_track* bdeck);

#endif
