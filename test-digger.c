#include <stdio.h>
#include "digger.h"

char* usb = "/media/null/22BC-F655/";

void test_get_playlists() {
  finyl_playlist* pls;
  
  int playlists_size = get_playlists(&pls, usb);

  for (int i = 0; i<playlists_size; i++) {
    printf("name: %s\n", pls[i].name);
    printf("id: %d\n", pls[i].id);
  }
}

void test_get_playlist_tracks() {
  finyl_track_meta* tracks;

  int tracks_size = get_playlist_tracks(&tracks, usb, 1);

  for (int i = 0; i<tracks_size; i++) {
    printf("id is %d\n", tracks[i].id);
    printf("bpm is %d\n", tracks[i].bpm);
    printf("title is %s\n", tracks[i].title);
    printf("title is %s\n", tracks[i].filepath);
  }
}

int main() {
  test_get_playlists();
}
