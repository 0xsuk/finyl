#ifndef WAVEFORM_H
#define WAVEFORM_H

#include "IWidget.h"
#include "finyl.h"
#include <SDL2/SDL.h>

class Interface;

class WaveForm : public IWidget {
 public:
  WaveForm(Interface& _interface);
  void draw();
  void set_range(int _range);
  void double_range();
  void half_range();
  bool render_adeck;
  bool render_bdeck;
  
 private:
  void erase_edge(int xdiff);
  void slide(SDL_Texture* texture, int xdiff);
  void draw_wave(finyl_track& t, int x, int mindex);
  void draw_waveform(SDL_Texture* texture, finyl_track& t, int starti, int nowi, int draw_range);
  int plus_waveform(SDL_Texture* texture, finyl_track& t, int nowi, int wave_y, int previ);
  void render_waveform(SDL_Texture* texture, finyl_track& t, int nowi, int wave_y);
  void draw_center_line();
  void draw_static_grids(finyl_track* t);
  void render_static_grids(SDL_Texture* texture, finyl_track* t, int wave_y);
  void render_deck(Deck& deck, int& previ, SDL_Texture* wavetx, int wave_y);
  void update_deck(Deck &deck, int &previ, SDL_Texture* wavetx, SDL_Texture* sgtx, int wave_y);
  
  int previ_adeck = 0;
  int previ_bdeck = 0;


  
  int wave_range;
  int wave_height;
  int wave_height_half;
  int wave_iteration_margin;
  SDL_Texture* tx_awave;
  SDL_Texture* tx_bwave;
  SDL_Texture* tx_asg; //static grid for A
  SDL_Texture* tx_bsg; //for B
  Interface& itf;
};


#endif
