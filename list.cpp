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

List::List(int _x, int _y, int _w, int _h, int _font_size):
  x(_x), y(_y), w(_w), h(_h),
  space(4),
  font_size(_font_size)
{
  font = TTF_OpenFont("DejaVuSans.ttf", font_size);
  if (font == nullptr) {
    printf("bad %s\n", TTF_GetError());
    return; 
  }
  init_select_tx();

}

void List::set_items(std::vector<std::string> _items) {
  head = 0;
  selected = 0;
  item_surfs.clear();
  item_txs.clear();
  
  items = std::move(_items);

  SDL_Color color = {255,255,255, 255};
  for (int i = 0; i<items.size(); i++) {
    item_surfs.push_back(TTF_RenderText_Solid(font, items[i].data(), color));
    
    item_txs.push_back(SDL_CreateTextureFromSurface(gApp.interface->renderer, item_surfs[i].get()));
    SDL_SetTextureBlendMode(item_txs[i].get(), SDL_BLENDMODE_BLEND);
  }

  for (;;visible_items++) {
    auto height_offset = (font_size+space)*visible_items;
    if (height_offset + font_size > h) {
      break;
    }
  }

}

void List::draw_item(SDL_Surface* surf, SDL_Texture* tx, int height_offset) {
  dest.x = x;
  dest.y = y + height_offset;
  dest.w = std::min(surf->w, w);
  dest.h = surf->h;

  src.x = 0;
  src.y = 0;
  src.w = w;
  src.h = surf->h;
  SDL_RenderCopy(gApp.interface->renderer, tx, &src, &dest);
}

void List::draw_select(int height_offset) {
  dest.x = x;
  dest.y = y + height_offset;
  dest.w = w;
  dest.h = font_size+space;
  
  SDL_RenderCopy(gApp.interface->renderer, select_tx.get(), NULL, &dest);
}

void List::draw() {
  for (int i = head; i<items.size(); i++) {
    if (!is_visible(i)) {
      break;
    }
    
    auto height_offset = get_height_offset(i);
    
    draw_item(item_surfs[i].get(), item_txs[i].get(), height_offset);
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
  if (selected > 0) {
    selected--;
    if (!is_visible(selected)) {
      head--;
    }
  }
}

void List::select_down() {
  if (selected < items.size()-1) {
    selected++;
    if (!is_visible(selected)) {
      head++;
    }
  }
}

int List::get_selected() {
  return selected;
}
