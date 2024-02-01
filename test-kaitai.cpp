#include <stdio.h>
#include <fstream>
#include "rekordbox_pdb.h"

// https://github.com/mixxxdj/mixxx/blob/034eb6c56a9af855ef5ce824841a113752ea4726/src/library/rekordbox/rekordboxfeature.cpp#L493

template<typename Base, typename T>
inline bool instanceof (const T* ptr) {
  return dynamic_cast<const Base*>(ptr) != nullptr;
}



std::string getText(rekordbox_pdb_t::device_sql_string_t* deviceString) {
  if (instanceof <rekordbox_pdb_t::device_sql_short_ascii_t>(deviceString->body())) {
    rekordbox_pdb_t::device_sql_short_ascii_t* shortAsciiString =
      static_cast<rekordbox_pdb_t::device_sql_short_ascii_t*>(deviceString->body());
    return shortAsciiString->text();
  } else if (instanceof <rekordbox_pdb_t::device_sql_long_ascii_t>(deviceString->body())) {
    rekordbox_pdb_t::device_sql_long_ascii_t* longAsciiString =
      static_cast<rekordbox_pdb_t::device_sql_long_ascii_t*>(deviceString->body());
    return longAsciiString->text();
  } else if (instanceof <rekordbox_pdb_t::device_sql_long_utf16be_t>(deviceString->body())) {
    rekordbox_pdb_t::device_sql_long_utf16be_t* longUtf16beString =
      static_cast<rekordbox_pdb_t::device_sql_long_utf16be_t*>(deviceString->body());
    printf("returning unicode\n");
    return longUtf16beString->text();
  }

  // Some strings read from Rekordbox *.PDB files contain random null characters
  // which if not removed cause Mixxx to crash when attempting to read file paths
  // return text.remove(QChar('\x0'));
  return "";
}

void printTrack(rekordbox_pdb_t::track_row_t* track) {
  int rbID = static_cast<int>(track->id());
  printf("track id: %d\n", rbID);
  printf("track title: %s\n", getText(track->title()).data());
}

void dostuff(rekordbox_pdb_t::page_type_t* tableOrder, int tableOrderIndex, std::vector<rekordbox_pdb_t::table_t*>::iterator table) {
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

              printf("PAGE_TYPE_KEYS: %s\n", getText(key->name()).data());
            } break;
            case rekordbox_pdb_t::PAGE_TYPE_GENRES: {
              // Genre found, update map
              rekordbox_pdb_t::genre_row_t* genre =
                static_cast<rekordbox_pdb_t::
                            genre_row_t*>(
                                          (*rowRef)->body());
              printf("PAGE_TYPE_GENRES: %s\n", getText(genre->name()).data());
            } break;
            case rekordbox_pdb_t::PAGE_TYPE_ARTISTS: {
              // Artist found, update map
              rekordbox_pdb_t::artist_row_t* artist =
                static_cast<rekordbox_pdb_t::
                            artist_row_t*>(
                                           (*rowRef)->body());
              printf("PAGE_TYPE_ARTISTS: %s\n", getText(artist->name()).data());
            } break;
            case rekordbox_pdb_t::PAGE_TYPE_ALBUMS: {
              // Album found, update map
              rekordbox_pdb_t::album_row_t* album =
                static_cast<rekordbox_pdb_t::
                            album_row_t*>(
                                          (*rowRef)->body());
              printf("PAGE_TYPE_ALBUMS: %s\n", getText(album->name()).data());
            } break;
            case rekordbox_pdb_t::PAGE_TYPE_PLAYLIST_ENTRIES: {
              // Playlist to track mapping found, update map
              rekordbox_pdb_t::playlist_entry_row_t*
                playlistEntry = static_cast<
                  rekordbox_pdb_t::
                  playlist_entry_row_t*>(
                                         (*rowRef)->body());
              printf("PAGE_TYPE_PLAYLIST_ENTRIES %d\n", playlistEntry->track_id());
            } break;
            case rekordbox_pdb_t::PAGE_TYPE_TRACKS: {
              printTrack(static_cast<rekordbox_pdb_t::track_row_t*>((*rowRef)->body()));
            } break;
            case rekordbox_pdb_t::PAGE_TYPE_PLAYLIST_TREE: {
              // Playlist tree node found, update map
              rekordbox_pdb_t::playlist_tree_row_t* playlistTree = static_cast<rekordbox_pdb_t::playlist_tree_row_t*>((*rowRef)->body());

              printf("PAGE_TYPE_PLAYLIST_TREE: %d\n", playlistTree->id());
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

int main() {
  std::string filepath = "/media/null/71CD-A534/PIONEER/rekordbox/export.pdb";
  std::ifstream ifs(filepath, std::ifstream::binary);
  kaitai::kstream ks(&ifs);

  rekordbox_pdb_t rekordboxDB = rekordbox_pdb_t(&ks);
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
  
  for (int tableOrderIndex = 0; tableOrderIndex < totalTables; tableOrderIndex++) {
    for (std::vector<rekordbox_pdb_t::table_t*>::iterator table = rekordboxDB.tables()->begin();
         table != rekordboxDB.tables()->end();
         ++table) {
      printf("enum : %d\n", (*table)->type());
      if ((*table)->type() != tableOrder[tableOrderIndex]) {
        continue;
      }

      dostuff(tableOrder, tableOrderIndex, table);
      
    }
  }
}
