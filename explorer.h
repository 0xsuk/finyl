#ifndef EXPLORER_H
#define EXPLORER_H

#include "IWidget.h"
#include "finyl.h"
#include "list.h"
#include <SDL2/SDL_ttf.h>
#include <vector>
#include <string>
#include "controller.h"

class Explorer : public IWidget {
public:
  Explorer();
  void draw();
  void select_down();
  void select_up();
  void select();
  void back();
  void load_track_2(Deck& deck);
private:
  void draw_title();
  
  void set_title(std::string title);
  void list_usb();
  void show_usb();
  void list_playlist();
  void show_playlist();
  void list_song();
  void show_song();
  
  int x;
  int y;
  int w;
  int h;
  
  List* active_list = &usb_list;
  
  SDL_Rect dest;
  
  Texture title_tx;
  Surface title_surf;
  TTF_Font* font;
  int font_size = 20;
  
  Usb* usb;
  int playlist_id;
  
  List usb_list;
  List playlist_list;
  List song_list;
  
  std::vector<std::string> usbs;
  std::map<int, std::string> playlists_map;
  std::vector<finyl_track_meta> songs_meta;
};

#endif
