#ifndef TEXT_H
#define TEXT_H

#include <string>
#include "sdl.h"
#include <SDL2/SDL_ttf.h>

class Text {
 public:
  Text(TTF_Font* font, int font_size, const std::string& text);
  std::string& get();
  void set_text(const std::string& _text);
  bool ready();
  void draw(const SDL_Rect* src, const SDL_Rect* dst);
 
  SDL_Surface* get_surf();
  void update();
 
 private:
  
  TTF_Font* font;
  int font_size = 17;
  std::string text = "";
  bool update_ = false;

  Texture tx;
  Surface surf;
};

#endif
