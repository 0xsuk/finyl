#include "cJSON.h"
#include "finyl.h"

#define BADGE_LENGTH 6

/* typedef struct { */
/*   char* usb; */
/*   char badge[BADGE_LENGTH]; */
/*   char* error; */
/*   int playlists_size; */
/*   int tracks_size; */
/*   playlist* playlists; */
/*   track_meta* tracks; */
/* } finyl_output; */

void free_playlists(finyl_playlist* pls, int size);

void free_track_metas(finyl_track_meta* tms, int size);

int get_playlists(finyl_playlist** pls, char* usb);

int get_playlist_tracks(finyl_track_meta** tracks, char* usb, int pid);

int get_track(finyl_track* t, char* usb, int tid);

int get_all_tracks(finyl_track** ts, char* usb);

cJSON* read_file_malloc_json(char* file);

