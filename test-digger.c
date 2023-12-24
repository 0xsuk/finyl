#include <stdio.h>
#include "digger.h"
#include "dev.h"

int main(int argc, char **argv) {
  if (argc < 2) {
    printf("usage ./test-controller <path to rekordbox usb: example /media/null/22BC-F655/ >\n");
    return 0;
  }
  
  usb = argv[1];

  finyl_track_meta* tms;
  int size = get_playlist_tracks(&tms, usb, 2);
  printf("size is %d\n", size);
  finyl_free_track_metas(tms, size);
}

