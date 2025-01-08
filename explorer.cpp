#include "explorer.h"
#include <SDL2/SDL_ttf.h>
#include "interface.h"

Explorer::Explorer():
  x(300),
  y(300),
  w(700),
  h(400),
  list_h{h-header_margin},
  menu_list{x, y+header_margin, w, list_h, font, font_size},
  usb_list{x, y+header_margin, w, list_h, font, font_size},
  playlist_list(x,y+header_margin,w, list_h, font, font_size),
  song_list(x,y+header_margin,w, list_h, font, font_size),
  title(font, font_size, "") {
  menus.push_back("usbs");
  menus.push_back("midi");
  
  if (font == nullptr) {
    printf("bad %s\n", TTF_GetError());
    return; 
  }

  list_menu();
}

void Explorer::select_up() {
  active_list->select_up();
}
void Explorer::select_down() {
  active_list->select_down();
}
void Explorer::select() {
  if (!active_list->has_item()) {
    return;
  }
  
  if (active_list == &menu_list) {
    std::string menu_name = menus[active_list->get_selected()];
    if (menu_name == "usbs") {
      list_usb();
    } else if (menu_name == "midi") {
      // list_midi();
    }
  } else if (active_list == &usb_list) {
    printf("selected %d\n", (int)active_list->get_selected());
    printf("len %d\n", (int)gApp.controller->usbs.size());
    usb = &gApp.controller->usbs[active_list->get_selected()];
    list_playlist();
  } else if (active_list == &playlist_list) {
    auto it = playlists_map.begin();
    std::advance(it, active_list->get_selected());
    playlist_id = it->first;
    list_song();
  }
}

void Explorer::load_track_2(Deck &deck) {
  load_track_n(deck, 2);
}

void Explorer::load_track_n(Deck& deck, int n) {
  if (active_list != &song_list) {
    return;
  }
  int track_id = songs_meta[active_list->get_selected()].id;
  gApp.controller->load_track_nstems(&deck.pTrack, track_id, deck.type, n);
}

void Explorer::load_track(Deck &deck) {
  if (active_list != &song_list) {
    return;
  }
  int track_id = songs_meta[active_list->get_selected()].id;
  gApp.controller->load_track(&deck.pTrack, track_id, deck.type);
}

void Explorer::back() {
  if (active_list == &menu_list) list_menu();
  else if (active_list == &usb_list) list_menu();
  else if (active_list == &playlist_list) list_usb();
  else if (active_list == &song_list) list_playlist();
}

void Explorer::list_menu() {
  title.set_text("menus");
  active_list = &menu_list;
  menu_list.set_items(menus);
}

void Explorer::list_usb() {
  gApp.controller->scan_usbs();
  usbs.clear();
  for (auto& usb: gApp.controller->usbs) {
    printf("usbroot:%s\n", usb.root.data());
    usbs.push_back(usb.root);
  }
  usb_list.set_items(usbs);

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

  title.set_text("playlists <" + usb->root);
  active_list = &playlist_list;
}

void Explorer::list_song() {
  std::vector<std::string> songs;

  getPlaylistTrackMetas(songs_meta, *usb, playlist_id);

  for (auto& meta: songs_meta) {
    songs.push_back(std::to_string(meta.bpm) + " " + meta.filename);
  }

  song_list.set_items(songs);
  
  auto it = playlists_map.find(playlist_id);
  title.set_text(it->second + " <" + usb->root);
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
