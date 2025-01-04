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
  void load_track_n(Deck& deck, int n);
  void load_track_2(Deck &deck);
  void load_track(Deck& deck);
  Usb& get_selected_usb() {
    return *usb;
  }

private:
  void draw_title();
  
  void set_title(std::string title);
  void list_device();
  void list_usb();
  void list_playlist();
  void list_song();
  
  int font_size = 20;
  int header_margin = font_size+5; //height of the header + some space
  int x;
  int y;
  int w;
  int h;
  int list_h; //height of the List component
  
  List* active_list = &usb_list;
  
  SDL_Rect dest;
  
  TTF_Font* font = TTF_OpenFont("NotoSansCJK-Regular.ttc", font_size);
  
  Usb* usb;
  int playlist_id;
  
  List device_list;
  List usb_list;
  List playlist_list;
  List song_list;
  
  Text title;
  
  std::vector<std::string> devices;
  std::vector<std::string> usbs;
  std::map<int, std::string> playlists_map;
  std::vector<finyl_track_meta> songs_meta;
};

#endif
