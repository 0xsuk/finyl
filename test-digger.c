#include <stdio.h>
#include "digger.h"
#include "dev.h"

void test_get_playlists() {
  finyl_playlist* pls;
  
  int playlists_size = get_playlists(&pls, usb);

  for (int i = 0; i<playlists_size; i++) {
    printf("name: %s\n", pls[i].name);
    printf("id: %d\n", pls[i].id);
  }
}

void test_get_playlist_tracks() {
  finyl_track_meta* tms;
  
  int tms_size = get_playlist_tracks(&tms, usb, 1);

  for (int i = 0; i<tms_size; i++) {
    print_track_meta(&tms[i]);
  }
}

void test_get_track() {
  finyl_track t;

  get_track(&t, usb, 1);

  print_track_meta(t.meta);
  printf("cues size is %d\n", t.cues_size);
  printf("cue1: %d\n", t.cues[1].time);
  printf("beats size is %d\n", t.beats_size);
}

/* void test_get_all_tracks() { */
/*   finyl_track* t; */

/*   get_all_tracks(&t, usb); */
/* } */

int main() {
  test_get_track();
  /* test_get_playlist_tracks(); */
  /* test_get_playlists(); */
}

