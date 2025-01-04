#include "list.h"
#include "finyl.h"
#include "interface.h"

void List::init_select_tx() {
  select_tx = SDL_CreateTexture(gApp.interface->renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, w, 1000);
  SDL_SetTextureBlendMode(select_tx.get(), SDL_BLENDMODE_BLEND);

  SDL_SetRenderTarget(gApp.interface->renderer, select_tx.get());
  SDL_SetRenderDrawColor(gApp.interface->renderer, 162, 162, 255, 100);
  SDL_RenderClear(gApp.interface->renderer);
  
  SDL_SetRenderTarget(gApp.interface->renderer, NULL);
}

List::List(int _x, int _y, int _w, int _h, TTF_Font* font, int _font_size):
  x(_x), y(_y), w(_w), h(_h),
  space(4),
  font(font),
  font_size(_font_size)
{
  if (font == nullptr) {
    printf("bad %s\n", TTF_GetError());
    return; 
  }
  init_select_tx();

}

void List::set_items(std::vector<std::string> _strings) {
  strings = std::move(_strings);
  update_items_ = true;

  no_item = strings.size() == 0;
}

//should NOT be called during draw() do stuff with texts, because update_items modify texts
void List::update_items() {
  head = 0;
  selected = 0;
  texts.clear();

  for (auto& str: strings) {
    texts.push_back(Text(font, font_size, str));
  }


  for (;;visible_items++) {
    auto height_offset = (font_size+space)*visible_items;
    if (height_offset + font_size > h) {
      break;
    }
  }
}

void List::draw_select(int height_offset) {
  dst.x = x;
  dst.y = y + height_offset;
  dst.w = w;
  dst.h = font_size+space;
  
  SDL_RenderCopy(gApp.interface->renderer, select_tx.get(), NULL, &dst);
}

void List::draw() {
  if (update_items_) {
    update_items();
    update_items_ = false;
  }
  
  for (int i = head; i<texts.size(); i++) {
    if (!is_visible(i)) {
      break;
    }
    auto& text = texts[i];
    
    text.update();
    if (!text.ready()) continue;
      
    auto height_offset = get_height_offset(i);
    dst.x = x;
    dst.y = y + height_offset;
    dst.w = std::min(text.get_surf()->w, w);
    dst.h = text.get_surf()->h;

    src.x = 0;
    src.y = 0;
    src.w = w;
    src.h = text.get_surf()->h;

    texts[i].draw(&src, &dst);
    
    if (i == selected) {
      draw_select(height_offset);
    }
  }
}

int List::get_height_offset(int index) {
  return (font_size+space) * (index - head);
}

bool List::is_visible(int index) {
  if (index < head) return false;
  if (index >= head+visible_items) return false;

  return true;
}

void List::select_up() {
  if (no_item) return;
  if (selected > 0) {
    selected--;
    if (!is_visible(selected)) {
      head--;
    }
  }
}

void List::select_down() {
  if (no_item) return;
  if (selected < texts.size()-1) {
    selected++;
    if (!is_visible(selected)) {
      head++;
    }
  }
}

bool List::has_item() {
  return !no_item;
}

int List::get_selected() {
  if (no_item) return -1;
  return selected;
}
