#include "text.h"
#include "interface.h"


Text::Text(TTF_Font* font, int font_size, const std::string& text): font(font), font_size(font_size), text(text) {
  update_ = true;
}

std::string& Text::get() {
  return text;
}

bool Text::ready() {
  if (surf.get() == nullptr) {
    return false;
  }
  
  if (tx.get() == nullptr) {
    return false;
  }
  
  return true;
}

void Text::set_text(const std::string &_text){
  text = _text;
  update_ = true;
}

void Text::update() {
  if (!update_) {
    return;
  }
  update_ = false;
  SDL_Color color = {255,255,255, 255};
  SDL_Surface* p;
  if (text == "") {
    p = TTF_RenderUTF8_Blended(font, " ", color);
  } else {
    p = TTF_RenderUTF8_Blended(font, text.data(), color);
  }
  if (p == nullptr) {
    return;
  }
  surf.reset(p);
  tx = SDL_CreateTextureFromSurface(gApp.interface->renderer, surf.get());
  SDL_SetTextureBlendMode(tx.get(), SDL_BLENDMODE_BLEND);
}

void Text::draw(const SDL_Rect* src, const SDL_Rect* dst) {
  SDL_RenderCopy(gApp.interface->renderer, tx.get(), src, dst);
}

SDL_Surface* Text::get_surf() {
  return surf.get();
}
