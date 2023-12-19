#ifndef INTERFACE_H
#define INTERFACE_H

#include "finyl.h"
#include <SDL2/SDL.h>

extern int amount;

int get_window_size();

void renderWaveform(SDL_Renderer *renderer, finyl_sample* pcmData, int numSamples);

int interface();

#endif
