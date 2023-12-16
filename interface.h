#ifndef INTERFACE_H
#define INTERFACE_H

#include "finyl.h"
#include <SDL2/SDL.h>

void renderWaveform(SDL_Renderer *renderer, finyl_sample* pcmData, int numSamples);

int interface(finyl_track* t);

#endif
