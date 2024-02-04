#include <stdio.h>
#include "rekordbox_pdb.h"
#include <fstream>
#include <string>
#include <map>
#include <array>
#include <algorithm>
#include "util.h"
#include "rekordbox.h"

namespace rekordbox {

std::vector<std::string> usbRoots;

template<typename Base, typename T>
inline bool instanceof (const T* ptr) {
  return dynamic_cast<const Base*>(ptr) != nullptr;
}

std::string getText(rekordbox_pdb_t::device_sql_string_t* deviceString) {
  std::string text;
  if (instanceof <rekordbox_pdb_t::device_sql_short_ascii_t>(deviceString->body())) {
    rekordbox_pdb_t::device_sql_short_ascii_t* shortAsciiString =
      static_cast<rekordbox_pdb_t::device_sql_short_ascii_t*>(deviceString->body());
    text = shortAsciiString->text();
  } else if (instanceof <rekordbox_pdb_t::device_sql_long_ascii_t>(deviceString->body())) {
    rekordbox_pdb_t::device_sql_long_ascii_t* longAsciiString =
      static_cast<rekordbox_pdb_t::device_sql_long_ascii_t*>(deviceString->body());
    text = longAsciiString->text();
  } else if (instanceof <rekordbox_pdb_t::device_sql_long_utf16le_t>(deviceString->body())) {
    text = static_cast<rekordbox_pdb_t::device_sql_long_utf16le_t*>(deviceString->body())->text();
  }

  // Some strings read from Rekordbox *.PDB files contain random null characters
  // which if not removed cause Mixxx to crash when attempting to read file paths
  // return text.remove(QChar('\x0'));
  text.erase(std::remove(text.begin(), text.end(), '\0'), text.end());
  return text;
}

//just do it :()
void dostuff(Usb& usb, rekordbox_pdb_t::page_type_t* tableOrder, int tableOrderIndex, std::vector<rekordbox_pdb_t::table_t*>::iterator table) {
  uint16_t lastIndex = (*table)->last_page()->index();
  rekordbox_pdb_t::page_ref_t* currentRef = (*table)->first_page();
  while (true) {
    rekordbox_pdb_t::page_t* page = currentRef->body();

    if (page->is_data_page()) {
      for (std::vector<rekordbox_pdb_t::row_group_t*>::
             iterator rowGroup =
             page->row_groups()->begin();
           rowGroup != page->row_groups()->end();
           ++rowGroup) {
        for (std::vector<rekordbox_pdb_t::row_ref_t*>::
               iterator rowRef = (*rowGroup)
               ->rows()
               ->begin();
             rowRef != (*rowGroup)->rows()->end();
             ++rowRef) {
          if ((*rowRef)->present()) {
            switch (tableOrder[tableOrderIndex]) {
            case rekordbox_pdb_t::PAGE_TYPE_KEYS: {
              // Key found, update map
              rekordbox_pdb_t::key_row_t* key =
                static_cast<rekordbox_pdb_t::
                            key_row_t*>(
                                        (*rowRef)->body());

              usb.musickeysMap[key->id()] = getText(key->name());
            } break;
            case rekordbox_pdb_t::PAGE_TYPE_GENRES: {
              // Genre found, update map
              rekordbox_pdb_t::genre_row_t* genre =
                static_cast<rekordbox_pdb_t::
                            genre_row_t*>(
                                          (*rowRef)->body());
              usb.genresMap[genre->id()] = getText(genre->name());
            } break;
            case rekordbox_pdb_t::PAGE_TYPE_ARTISTS: {
              // Artist found, update map
              rekordbox_pdb_t::artist_row_t* artist =
                static_cast<rekordbox_pdb_t::
                            artist_row_t*>(
                                           (*rowRef)->body());
              usb.artistsMap[artist->id()] = getText(artist->name());
            } break;
            case rekordbox_pdb_t::PAGE_TYPE_ALBUMS: {
              // Album found, update map
              rekordbox_pdb_t::album_row_t* album =
                static_cast<rekordbox_pdb_t::
                            album_row_t*>(
                                          (*rowRef)->body());
              usb.albumsMap[album->id()] = getText(album->name());
            } break;
            case rekordbox_pdb_t::PAGE_TYPE_PLAYLIST_ENTRIES: {
              // Playlist to track mapping found, update map
              rekordbox_pdb_t::playlist_entry_row_t*
                playlistEntry = static_cast<
                  rekordbox_pdb_t::
                  playlist_entry_row_t*>(
                                         (*rowRef)->body());
              usb.playlistTrackMap[playlistEntry->playlist_id()][playlistEntry->entry_index()] = playlistEntry->track_id();
            } break;
            case rekordbox_pdb_t::PAGE_TYPE_TRACKS: {
              auto* t = static_cast<rekordbox_pdb_t::track_row_t*>((*rowRef)->body());
              
              usb.tracksMap[t->id()] = TrackRow{.title=getText(t->title()),
                                             .filename=getText(t->filename()),
                                             .relativeFilepath=getText(t->file_path()),
                                             .anlzFilepath=getText(t->analyze_path()),
                                             .tempo=(int)t->tempo(),
                                             .musickey=(int)t->key_id(),
                                             .genre=(int)t->genre_id(),
                                             .artist=(int)t->artist_id(),
              };
            } break;
            case rekordbox_pdb_t::PAGE_TYPE_PLAYLIST_TREE: {
              // Playlist tree node found, update map
              rekordbox_pdb_t::playlist_tree_row_t* playlistTree = static_cast<rekordbox_pdb_t::playlist_tree_row_t*>((*rowRef)->body());

              usb.playlistNamesMap[playlistTree->id()] = getText(playlistTree->name());
            
            } break;
            default:
              break;
            }
          }
        }
      }
    }

    if (currentRef->index() == lastIndex) {
      break;
    } else {
      currentRef = page->next_page();
    }
  }
}

constexpr int totalTables = 8;
rekordbox_pdb_t::page_type_t tableOrder[totalTables] = {
  rekordbox_pdb_t::PAGE_TYPE_KEYS,
  rekordbox_pdb_t::PAGE_TYPE_GENRES,
  rekordbox_pdb_t::PAGE_TYPE_ARTISTS,
  rekordbox_pdb_t::PAGE_TYPE_ALBUMS,
  rekordbox_pdb_t::PAGE_TYPE_PLAYLIST_ENTRIES,
  rekordbox_pdb_t::PAGE_TYPE_TRACKS,
  rekordbox_pdb_t::PAGE_TYPE_PLAYLIST_TREE,
  rekordbox_pdb_t::PAGE_TYPE_HISTORY};


void unplug(Usb& usb) {
  auto it = std::find(usbRoots.begin(), usbRoots.end(), usb.root);
  if (it != usbRoots.end()) {
    usbRoots.erase(it);
  }
}


int plug(Usb& usb) {
  auto it = std::find(usbRoots.begin(), usbRoots.end(), usb.root);
  if (it != usbRoots.end()) {
    return 0;
  }
  
  usbRoots.push_back(usb.root);
  
  std::string filepath = join_path(usb.root.data(), "/PIONEER/rekordbox/export.pdb");
  std::ifstream ifs(filepath, std::ifstream::binary);
  kaitai::kstream ks(&ifs);

  rekordbox_pdb_t rekordboxDB = rekordbox_pdb_t(&ks);

  for (int tableOrderIndex = 0; tableOrderIndex < totalTables; tableOrderIndex++) {
    for (std::vector<rekordbox_pdb_t::table_t*>::iterator table = rekordboxDB.tables()->begin();
         table != rekordboxDB.tables()->end();
         ++table) {
      if ((*table)->type() != tableOrder[tableOrderIndex]) {
        continue;
      }

      dostuff(usb, tableOrder, tableOrderIndex, table);
    }
  }
  
  return 0;
}

error getPlaylistTracks(std::vector<finyl_track_meta>& tms, const Usb& usb, int playlistid) {
  auto playlist = usb.playlistTrackMap.find(playlistid);
  if (playlist == usb.playlistTrackMap.end()) return noerror;
  const auto& map = playlist->second;
  
  tms.resize(map.size());
  int i = 0;
  for (auto& t: map) {
    int trackId = t.second;
    const auto& trackRow = usb.tracksMap.find(trackId)->second;
    tms[i].id = trackId;
    tms[i].bpm = trackRow.tempo;
    tms[i].title = trackRow.title;
    tms[i].filename = trackRow.filename;
    tms[i].filepath = trackRow.relativeFilepath;
    tms[i].musickeyid = trackRow.musickey;
    i++;
  }
  return noerror;
}

}
