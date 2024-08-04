#ifndef LIST_H
#define LIST_H

#include "IWidget.h"
#include <SDL2/SDL_ttf.h>
#include <vector>
#include <string>


#include "sdl.h"

class List : public IWidget {
public:
  List(int _x, int _y, int _w, int _h, int _font_size);
  
  void draw();
  void set_items(std::vector<std::string> _items);
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
  std::vector<Surface> item_surfs;
  std::vector<Texture> item_txs;
  Texture select_tx; //indicates item is selected
  TTF_Font* font;
};


#endif
