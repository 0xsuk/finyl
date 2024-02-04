#ifndef REKORDBOX_H
#define REKORDBOX_H

#include <string>
#include <map>
#include "finyl.h"

namespace rekordbox {

struct TrackRow {
  std::string title;
  std::string filename;
  std::string relativeFilepath;
  std::string anlzFilepath;
  int tempo;
  int musickey;
  int genre;
  int artist;
};

struct Usb {
  std::string root;
  std::map<int, std::string> musickeysMap;
  std::map<int, std::string> genresMap;
  std::map<int, std::string> artistsMap;
  std::map<int, std::string> albumsMap;
  std::map<int, std::map<int, int>> playlistTrackMap; //<playlistId, <entryIndex, trackId> where entryIndex is a relative index of a track in a playlist
  std::map<int, std::string> playlistNamesMap;
  std::map<int, TrackRow> tracksMap;

  Usb() = default;
  Usb(Usb&&) = default;
  Usb& operator=(Usb&&) = default;
};
void printTrackName(const Usb& usb, int tid);
void printPlaylistTracks(const Usb& usb, int playlist_id);
void unplug(Usb& usb);
int plug(Usb& usb);
error getPlaylistTracks(std::vector<finyl_track_meta>& tms, const Usb& usb, int playlistid);

}

#endif
