#include "explorer.h"
#include <SDL2/SDL_ttf.h>
#include "interface.h"

Explorer::Explorer():
  x(300),
  y(300),
  w(500),
  h(400),
  usb_list{x, y+20, w, h, font_size},
  playlist_list(x,y+20,w,h, font_size),
  song_list(x,y+20,w,h, font_size) {
  font = TTF_OpenFont("DejaVuSans.ttf", font_size);
  if (font == nullptr) {
    printf("bad %s\n", TTF_GetError());
    return; 
  }

  show_usb();
}

void Explorer::select_up() {
  active_list->select_up();
}
void Explorer::select_down() {
  active_list->select_down();
}
void Explorer::select() {
  if (active_list == &usb_list) {
    usb = &gApp.controller->usbs[active_list->get_selected()];
    show_playlist();
  } else if (active_list == &playlist_list) {
    auto it = playlists_map.begin();
    std::advance(it, active_list->get_selected());
    playlist_id = it->first;
    show_song();
  } else if (active_list == &song_list) {
    int track_id = songs_meta[active_list->get_selected()].id;
    gApp.controller->load_track_nstems(&gApp.controller->adeck->pTrack, track_id, finyl_a, 2);
  }
}
void Explorer::back() {
  if (active_list == &usb_list) return;
  else if (active_list == &playlist_list) show_usb();
  else if (active_list == &song_list) show_playlist();
}

void Explorer::list_usb() {
  usbs.clear();
  
  for (auto& usb: gApp.controller->usbs) {
    usbs.push_back(usb.root);
  }

  usb_list.set_items(usbs);
}

void Explorer::show_usb() {
  if (usbs.size()==0) list_usb();
  set_title("usb:");
  active_list = &usb_list;
}

void Explorer::list_playlist() {
  
  std::vector<std::string> playlists;
  for (auto it = usb->playlistNamesMap.begin(); it != usb->playlistNamesMap.end(); it++) {
    playlists_map[it->first] = it->second.data();
    playlists.push_back(it->second.data());
  }

  playlist_list.set_items(playlists);
}

void Explorer::show_playlist() {
  if (playlists_map.size() == 0) list_playlist();
  set_title("playlist:");
  active_list = &playlist_list;
}

void Explorer::list_song() {
  std::vector<std::string> songs;

  getPlaylistTrackMetas(songs_meta, *usb, playlist_id);

  for (auto& meta: songs_meta) {
    songs.push_back(std::to_string(meta.bpm) + " " + meta.title);
  }

  song_list.set_items(songs);
}

void Explorer::show_song() {
  list_song();

  set_title("song:");
  active_list = &song_list;
}


void Explorer::set_title(std::string title) {
  SDL_Color color = {255,255,255, 255};
  title_surf = Surface(TTF_RenderText_Solid(font, title.data(), color));
  title_tx = SDL_CreateTextureFromSurface(gApp.interface->renderer, title_surf.get());
}

void Explorer::draw_title() {
  dest.x = x;
  dest.y = y;
  dest.w = std::min(title_surf.get()->w, w);
  dest.h = title_surf.get()->h;
  
  SDL_RenderCopy(gApp.interface->renderer, title_tx.get(), NULL, &dest);
}

void Explorer::draw() {
  draw_title();
  active_list->draw();
}
