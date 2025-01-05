#ifndef INTERFACE_H
#define INTERFACE_H

#include "finyl.h"
#include <SDL2/SDL.h>
#include "waveform.h"
#include "explorer.h"
#include "controller.h"
#include "gif.h"
#include "spectrum.h"


class Interface {
public:
  Interface();
  int run();

  int win_width = 1000;
  int win_height = 500;
  std::unique_ptr<Waveform> waveform;
  std::unique_ptr<Explorer> explorer;
  std::unique_ptr<GIF> cat;
  std::unique_ptr<GIF> pikachu;
  std::unique_ptr<Spectrum> a_spectrum;
  std::unique_ptr<Spectrum> b_spectrum;
  
  SDL_Renderer* renderer;
  SDL_Window* window;
  

  void add_track_to_free(finyl_track* t);
  //interface frees track when done with it
  void free_tracks();
  void set_fps(int _fps) {
    fps = _fps;
  }

private:
  int fps;
  finyl_track* tracks_to_free[2];
  int tracks_to_free_tail = 0;
};
#endif
