#define BADGE_LENGTH 6

typedef struct {
  int id;
  char name[300];
} playlist;

typedef struct {
  int id;
  int bpm;
  int musickeyid;
  int filesize;
  char title[300];
  char filepath[1000];
  char filename[300];
} digger_track;

typedef struct {
  char usb[300];
  char badge[BADGE_LENGTH];
  char* error;
  int playlists_length;
  playlist playlists[100];
  digger_track* tracks;
} finyl_output;
