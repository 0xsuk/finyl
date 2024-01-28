#ifndef INTERFACE_H
#define INTERFACE_H

#include "finyl.h"
#include <SDL2/SDL.h>

struct Interface {
  int win_width = 1000;
  int win_height = 500;
  int wave_range = 1000000;
  int wave_height = 100;
  int wave_height_half = 50;
  int wave_iteration_margin = 100;
  bool render_adeck = false;
  bool render_bdeck = false;

  SDL_Renderer* renderer = nullptr;
  SDL_Texture* tx_awave = nullptr;
  SDL_Texture* tx_bwave = nullptr;
  SDL_Texture* tx_asg = nullptr; //static grid for A
  SDL_Texture* tx_bsg = nullptr; //for B
};

extern Interface interface;

void add_track_to_free(finyl_track* t);

void free_tracks();

void set_wave_range(Interface& itf, int wave_range);

int run_interface();

#endif
