#ifndef REKORDBOX_H
#define REKORDBOX_H

#include <string>
#include <map>
#include "finyl.h"

struct TrackRow {
  std::string title;
  std::string filename;
  std::string relativeFilepath;
  std::string anlzRelativeFilepath;
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

  Usb(const std::string& _root) {
    root = _root;
  }
};

void printPlaylists(const Usb& usb);
void printPlaylistTracks(const Usb& usb, int playlistid);
int plug(const std::string& root);
void getPlaylistTrackMetas(std::vector<finyl_track_meta>& tms, const Usb& usb, int playlistid);
void getTrackMeta1(finyl_track_meta& tm, const Usb& usb, int trackId);
void getTrackMeta(finyl_track_meta& tm, const Usb& usb, int trackId);
void readAnlz(Usb& usb, finyl_track& t, int trackId);

#endif
