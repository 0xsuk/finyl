#define BADGE_LENGTH 6

typedef struct {
  int id;
  char* name;
} playlist;

typedef struct {
  int id;
  int bpm;
  int musickeyid;
  int filesize;
  char* title;
  char* filepath;
  char* filename;
} track_meta; //used in track listing in playlist

typedef struct {
  char* usb;
  char badge[BADGE_LENGTH];
  char* error;
  int playlists_size;
  int tracks_size;
  playlist* playlists;
  track_meta* tracks;
} finyl_output;

void free_finyl_output(finyl_output* fo);
