#include <dirent.h>
#include <stdio.h>
#include "rekordbox_pdb.h"
#include "rekordbox_anlz.h"
#include <fstream>
#include <string>
#include <algorithm>
#include "util.h"
#include "dev.h"
#include "extern.h"
#include "rekordbox.h"

std::vector<Usb> usbs;

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
                                             .anlzRelativeFilepath=getText(t->analyze_path()),
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
  for (auto it = usbs.begin(); it!=usbs.end();it++) {
    if (usb.root == it->root) {
      printf("unplugged %s\n", it->root.data());
      usbs.erase(it);
    }
  }
}


int plug(const std::string& root) {
  for (auto& u: usbs) {
    if (u.root == root) {
      printf("already plugged\n");
      return 0;
    }
  }
  
  Usb usb(root);
  
  std::string filepath = join_path(usb.root.data(), "/PIONEER/rekordbox/export.pdb");
  std::ifstream ifs(filepath, std::ifstream::binary);
  if (!ifs) {
    printf("usb does not exist\n");
    return 1;
  }
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

  usbs.push_back(std::move(usb));
  return 0;
}

bool match(std::string_view hash, char* filename) {
  int dot = find_char_last(filename, '.');
  if (dot == -1) {
    return false;
  }
  char f[dot+1];
  strncpy(f, filename, dot);
  f[dot] = '\0';

  int hyphen = find_char_last(f, '-');
  if (hyphen == -1) {
    return false;
  }
  
  if (dot - hyphen == 33) {
    if (strncmp(hash.data(), &filename[hyphen+1], 32) == 0) {
      return true;
    }
  }
  return false;
}

void set_channels_filepaths(finyl_track_meta& tm, std::string_view root) {
  std::string hash = compute_md5(tm.filepath);
  std::string model_dir = join_path(root.data(), "finyl/separated/hdemucs_mmi");
  
  DIR* d = opendir(model_dir.data());
  struct dirent* dir;
  if (d) {
    while ((dir = readdir(d)) != NULL) {
      if (match(hash, dir->d_name)) {
        auto f = join_path(model_dir.data(), dir->d_name);
        tm.stem_filepaths.push_back(std::move(f));
      }
    }
    closedir(d);
  }
}

//execpt stemfiles
void getTrackMeta1(finyl_track_meta& tm, const Usb& usb, int trackId) {
  const auto& trackRow = usb.tracksMap.find(trackId)->second;
  tm.id = trackId;
  tm.bpm = trackRow.tempo;
  tm.title = trackRow.title;
  tm.filename = trackRow.filename;
  tm.filepath = join_path(usb.root.data(), trackRow.relativeFilepath.data());
  tm.musickeyid = trackRow.musickey;
}

//include stemfiles
void getTrackMeta(finyl_track_meta& tm, const Usb& usb, int trackId) {
  getTrackMeta1(tm, usb, trackId);
  set_channels_filepaths(tm, usb.root);
}

void getPlaylistTrackMetas(std::vector<finyl_track_meta>& tms, const Usb& usb, int playlistid) {
  auto playlist = usb.playlistTrackMap.find(playlistid);
  if (playlist == usb.playlistTrackMap.end()) return;
  const auto& map = playlist->second;
  
  tms.resize(map.size());
  int i = 0;
  for (auto& t: map) {
    int trackId = t.second;
    getTrackMeta1(tms[i], usb, trackId);
    i++;
  }
}

static void readBeatGrids(finyl_track& t, rekordbox_anlz_t::beat_grid_tag_t* beatGridTag) {
  for (std::vector<rekordbox_anlz_t::beat_grid_beat_t*>::iterator
         beat = beatGridTag->beats()->begin();
       beat != beatGridTag->beats()->end();
       ++beat) {
    
    t.beats.push_back(finyl_beat{
        .time=static_cast<int>((*beat)->time()),
        .number=(*beat)->beat_number()
        });
    // int time = static_cast<int>((*beat)->time()) - timingOffset;
    // // Ensure no offset times are less than 1
    // if (time < 1) {
    //   time = 1;
    // }
    // beats << mixxx::audio::FramePos(sampleRateKhz * static_cast<double>(time));
  }
}

static void readCueTags(finyl_track& t, rekordbox_anlz_t::cue_tag_t* cuesTag) {
  for (std::vector<rekordbox_anlz_t::cue_entry_t*>::iterator
         cueEntry = cuesTag->cues()->begin();
       cueEntry != cuesTag->cues()->end();
       ++cueEntry) {
    int time = static_cast<int>((*cueEntry)->time());

    switch (cuesTag->type()) {
    case rekordbox_anlz_t::CUE_LIST_TYPE_MEMORY_CUES: {
      // switch ((*cueEntry)->type()) {
      // case rekordbox_anlz_t::CUE_ENTRY_TYPE_MEMORY_CUE: {
      // } break;
      // case rekordbox_anlz_t::CUE_ENTRY_TYPE_LOOP: {
      // } break;
      // } break;
      t.cues.push_back(finyl_cue{
          .type=CUE::MEMORY_CUE,
          .time=(double)time
        });
    }
    case rekordbox_anlz_t::CUE_LIST_TYPE_HOT_CUES: {
      
    } break;
    }
    // rekordbox_anlz_t::CUE_LIST_TYPE_MEMORY_CUES
    // rekordbox_anlz_t::CUE_ENTRY_TYPE_LOOP
    //rekordbox_anlz_t::CUE_LIST_TYPE_HOT_CUES
    // Ensure no offset times are less than 1
    
  }

  if (t.cues.size() > 0) {
    // 1000 milisec = 44100 samples
    // time milisec = 44100 / 1000 * time samples
    t.loop_in = (sample_rate / 1000.0) * (double)t.cues[0].time;
    t.cues[0].type = CUE::ACTIVE_MEMORY_CUE;
  }
}

static void readCueTags2(finyl_track& t, rekordbox_anlz_t::cue_extended_tag_t* cuesExtendedTag) {
  for (std::vector<rekordbox_anlz_t::cue_extended_entry_t*>::iterator
         cueExtendedEntry = cuesExtendedTag->cues()->begin();
       cueExtendedEntry != cuesExtendedTag->cues()->end();
       ++cueExtendedEntry) {
    int time = static_cast<int>((*cueExtendedEntry)->time());
    auto type = cuesExtendedTag->type();
    printf("CUE2 time is %d\n", time);
    // rekordbox_anlz_t::CUE_LIST_TYPE_MEMORY_CUES
    // rekordbox_anlz_t::CUE_ENTRY_TYPE_LOOP
    // rekordbox_anlz_t::CUE_LIST_TYPE_HOT_CUES
  }
}

// void unknownFunc() {
//   std::sort(memoryCuesAndLoops.begin(),
//             memoryCuesAndLoops.end(),
//             [](const memory_cue_loop_t& a, const memory_cue_loop_t& b)
//             -> bool { return a.startPosition < b.startPosition; });

//   bool mainCueFound = false;

//   // Add memory cues and loops
//   for (int memoryCueOrLoopIndex = 0;
//        memoryCueOrLoopIndex < memoryCuesAndLoops.size();
//        memoryCueOrLoopIndex++) {
//     memory_cue_loop_t memoryCueOrLoop = memoryCuesAndLoops[memoryCueOrLoopIndex];

//     if (!mainCueFound && !memoryCueOrLoop.endPosition.isValid()) {
//       // Set first chronological memory cue as Mixxx MainCue
//       track->setMainCuePosition(memoryCueOrLoop.startPosition);
//       CuePointer pMainCue = track->findCueByType(mixxx::CueType::MainCue);
//       pMainCue->setLabel(memoryCueOrLoop.comment);
//       pMainCue->setColor(*memoryCueOrLoop.color);
//       mainCueFound = true;
//     } else {
//       // Mixxx v2.4 will feature multiple loops, so these saved here will be usable
//       // For 2.3, Mixxx treats them as hotcues and the first one will be loaded as the single loop Mixxx supports
//       lastHotCueIndex++;
//       setHotCue(
//                 track,
//                 memoryCueOrLoop.startPosition,
//                 memoryCueOrLoop.endPosition,
//                 lastHotCueIndex,
//                 memoryCueOrLoop.comment,
//                 memoryCueOrLoop.color);
//     }
//   }
  
// }

static void readAnlz(finyl_track& t, const std::string& anlzPath) {
  std::ifstream ifs(anlzPath, std::ifstream::binary);
  kaitai::kstream ks(&ifs);

  auto anlz = rekordbox_anlz_t(&ks);
  
  for (std::vector<rekordbox_anlz_t::tagged_section_t*>::iterator section = anlz.sections()->begin(); section != anlz.sections()->end(); ++section) {
    switch ((*section)->fourcc()) {
    case rekordbox_anlz_t::SECTION_TAGS_BEAT_GRID: {
      printf("beatgrid\n");
      // if (!ignoreCues) {
        // break;
      // }

      readBeatGrids(t, static_cast<rekordbox_anlz_t::beat_grid_tag_t*>((*section)->body()));
    } break;
    case rekordbox_anlz_t::SECTION_TAGS_CUES: {
      // if (ignoreCues) {
      //   break;
      // }

      readCueTags(t, static_cast<rekordbox_anlz_t::cue_tag_t*>((*section)->body()));
    } break;
    case rekordbox_anlz_t::SECTION_TAGS_CUES_2: {
      // if (ignoreCues) {
      //   break;
      // }

      readCueTags2(t, static_cast<rekordbox_anlz_t::cue_extended_tag_t*>((*section)->body()));
    } break;
    default:
      break;
    }
  }

  // if (memoryCuesAndLoops.size() > 0) {
  //   unkownFunc();
  // }
}

void readAnlz(Usb& usb, finyl_track& t, int trackId) {
  for (auto& tr: usb.tracksMap) {
    if (tr.first == trackId) {
      std::string full = join_path(usb.root.data(), tr.second.anlzRelativeFilepath.data());
      readAnlz(t, full);
      return;
    }
  }
}

void printPlaylists(const Usb& usb) {
  for (auto it = usb.playlistNamesMap.begin(); it != usb.playlistNamesMap.end(); it++) {
    printf("%d: %s\n", it->first, it->second.data());
  }
}

void printPlaylistTracks(const Usb& usb, int playlistid) {
  std::vector<finyl_track_meta> tms;
  getPlaylistTrackMetas(tms, usb, playlistid);

  for (auto& tm: tms) {
    printf("%d:(%d) %s\n", tm.id, tm.bpm, tm.title.data());
  }
}
