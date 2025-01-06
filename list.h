#ifndef LIST_H
#define LIST_H

#include "IWidget.h"
#include <SDL2/SDL_ttf.h>
#include <vector>
#include <string>


#include "sdl.h"
#include "text.h"

class List : public IWidget {
public:
  List(int _x, int _y, int _w, int _h, TTF_Font* font, int _font_size);
  
  void draw();
  void set_items(std::vector<std::string> _items);
  void select_down();
  void select_up();
  int get_selected();
  const std::vector<int>& get_marked();
  void toggle_mark();
  bool has_item();
  
private:
  void update_items();
  void draw_item(SDL_Surface* surf, SDL_Texture* tx, int height_offset);
  void draw_select(int height_offset);
  void draw_mark(int height_offset);
  void init_select_tx();
  bool is_visible(int index);
  int get_height_offset(int index);
  
  bool no_item = false;
  int x_mark;
  int x_list;
  int y;
  int w;
  int w_list;
  int h;
  int space; //between lines
  TTF_Font* font;
  int font_size;
  SDL_Rect dst;
  SDL_Rect src;
  int selected = 0;
  int head = 0; //head of visible list
  int visible_items = 0;
  std::vector<int> marked;

  bool update_items_ = false;
  
  std::vector<std::string> strings;
  std::vector<Text> texts;
  Texture select_tx; //indicates item is selected
};


#endif
