#include "explorer.h"
#include <SDL2/SDL_ttf.h>
#include "interface.h"

Explorer::Explorer():
  x(300),
  y(300),
  w(1000),
  h(700),
  usb_list{x, y+20, w, h, font, font_size},
  playlist_list(x,y+20,w,h, font, font_size),
  song_list(x,y+20,w,h, font, font_size),
  title(font, font_size, "") {
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
  }
}

void Explorer::load_track_2(Deck& deck) {
  if (active_list != &song_list) {
    return;
  }
  int track_id = songs_meta[active_list->get_selected()].id;
  gApp.controller->load_track_nstems(&deck.pTrack, track_id, deck.type, 2);
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
  title.set_text("usb");
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
  title.set_text("playlists");
  active_list = &playlist_list;
}

void Explorer::list_song() {
  std::vector<std::string> songs;

  getPlaylistTrackMetas(songs_meta, *usb, playlist_id);

  for (auto& meta: songs_meta) {
    songs.push_back(std::to_string(meta.bpm) + " " + meta.filename);
  }

  song_list.set_items(songs);
}

void Explorer::show_song() {
  list_song();

  auto it = playlists_map.find(playlist_id);
  title.set_text(it->second);
  active_list = &song_list;
}


void Explorer::draw() {
  title.update();
  if (title.ready()) {
    dest.x = x;
    dest.y = y;
    dest.w = std::min(title.get_surf()->w, w);
    dest.h = title.get_surf()->h;
    title.draw(NULL, &dest);
  }
  
  active_list->draw();
}
