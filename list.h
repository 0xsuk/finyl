#ifndef LIST_H
#define LIST_H

#include "IWidget.h"
#include <SDL2/SDL_ttf.h>
#include <vector>
#include <string>

class List : public IWidget {
public:
  List();
  
  void draw();
  void select_down();
  void select_up();
  int get_selected();
  
private:
  void draw_item(SDL_Surface* surf, SDL_Texture* tx, int height_offset);
  void draw_select(int height_offset);
  void init_select_tx();
  bool is_visible(int index);
  int get_height_offset(int index);
  
  int x;
  int y;
  int w;
  int h;
  int space; //between lines
  int font_size;
  SDL_Rect dest;
  SDL_Rect src;
  int selected = 0;
  int head = 0; //head of visible list
  int visible_items = 0;

  std::vector<std::string> items;
  std::vector<SDL_Surface*> item_surfs;
  std::vector<SDL_Texture*> item_txs;
  SDL_Texture* select_tx; //indicates item is selected
};


#endif
