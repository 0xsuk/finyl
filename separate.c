#include "finyl.h"
#include "dev.h"
#include "digger.h"
#include <stdlib.h>

#define same(x,y) (strcmp(x, y) == 0)

void separate_track(finyl_track_meta* tm) {
  char command[1000];
  char md5[33];
  compute_md5(tm->filepath, md5);
  snprintf(command, sizeof(command), "demucs -n hdemucs_mmi --two-stems=vocals \"%s\" -o %s/finyl/separated --filename {track}-{stem}-%s.{ext} --mp3", tm->filepath, usb, md5);
  printf("\nRunning:\n");
  printf("\t%s\n\n", command);
}

void demucs_track(char* tid) {
  int _tid = atoi(tid);
  finyl_track t;
  finyl_init_track(&t);
  if (get_track(&t, usb, _tid) == -1) {
    printf("abort\n");
    return;
  }

  separate_track(&t.meta);
}

void demucs_playlists(char* pid) {
  finyl_track_meta* tms;
  int _pid = atoi(pid);
  int tms_size = get_playlist_tracks(&tms, usb, _pid);
  if (tms_size == -1) {
    return;
  }
  
  for (int i = 0; i<tms_size; i++) {
    separate_track(&tms[i]);
  }
    
  finyl_free_track_metas(tms, tms_size);
}

void print_usage() {
  printf("usage: ./separate <usb> <operation>\n\n");
  printf("operation:\n");
  printf("\tlist-playlists: list playlists\n");
  printf("\tlist-playlist-tracks <id>: list tracks in playlist <id>\n");
  printf("\tdemucs-playlists <id>: run demucs for playlist <id>\n");
  printf("\tdemucs-track <id>: run demucs for track <id>\n");
}

int main(int argc, char **argv) {
  if (argc < 3) {
    print_usage();
    return 0;
  }
  
  usb = argv[1];
  
  char* op = argv[2];

  if (same(op, "list-playlists")) {
    list_playlists();
  } else if (same(op, "list-playlist-tracks")) {
    if (argc < 4) {
      print_usage();
      return 0;
    }

    int id = atoi(argv[3]);
    list_playlist_tracks(id);
  } else if (same(op, "demucs-playlists")) {
    if (argc < 4) {
      print_usage();
      return 0;
    }
    char* id = argv[3];
    demucs_playlists(id);
  } else if (same(op, "demucs-track")) {
    if (argc < 4) {
      print_usage();
      return 0;
    }

    char* id = argv[3];
    demucs_track(id);
  } else {
    print_usage();
  }
}
