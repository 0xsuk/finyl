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

int get_playlists(finyl_playlist* pls, char* usb);

cJSON* read_file_malloc_json(char* file);

