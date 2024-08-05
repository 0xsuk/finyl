#ifndef INTERFACE_H
#define INTERFACE_H

#include "finyl.h"
#include <SDL2/SDL.h>
#include "waveform.h"
#include "explorer.h"
#include "controller.h"
#include "gif.h"


class Interface {
public:
  int run();

  int win_width = 1000;
  int win_height = 500;
  std::unique_ptr<WaveForm> waveform;
  std::unique_ptr<Explorer> explorer;
  std::unique_ptr<GIF> cat;
  std::unique_ptr<GIF> pikachu;
  
  SDL_Renderer* renderer;
  SDL_Window* window;
  

  void add_track_to_free(finyl_track* t);
  //interface frees track when done with it
  void free_tracks();
  void set_fps(int _fps) {
    fps = _fps;
  }

private:
  int fps = 30;
  finyl_track* tracks_to_free[2];
  int tracks_to_free_tail = 0;
};
#endif
