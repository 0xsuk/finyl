#ifndef WAVEFORM_H
#define WAVEFORM_H

#include "IWidget.h"
#include "finyl.h"
#include <SDL2/SDL.h>
#include "sdl.h"

class Wave {
public:
  Wave(Deck& deck, int wave_width, int wave_y, int wave_range, int wave_height, int wave_iteration_margin);
  void set_range(int _range);
  void double_range();
  void half_range();
  bool init = false;
  void draw();
  
private:
  void erase_edge(int xdiff);
  void slide(SDL_Texture* texture, int xdiff);
  void draw_sample(finyl_track& t, int x, int mindex);
  void draw_wave(finyl_track& t, int starti, int nowi, int draw_range);
  void update_wave(finyl_track& t);
  void init_wave(finyl_track& t);
  void draw_center_line();
  void draw_static_grids_(finyl_track& t);
  void draw_static_grids(finyl_track& t);
  void render_static_grids(SDL_Texture* texture, finyl_track* t, int wave_y);
  void render_deck(Deck& deck, int& previ, SDL_Texture* wavetx, int wave_y);
  void update_deck(Deck &deck, int &previ, SDL_Texture* wavetx, SDL_Texture* sgtx, int wave_y);

  
  int wave_width;
  int wave_y;
  int wave_range;
  int wave_height;
  int wave_height_half;
  int wave_iteration_margin;
  int previ;
  Texture tx_wave;
  Texture tx_sg;
  Deck& deck;
};

class Waveform {
 public:
  Waveform();
  void draw();
  void set_range(int _range);
  void double_range();
  void half_range();
  
  Wave a_wave;
  Wave b_wave;
  
};


#endif
