#include "cJSON.h"
#include "finyl.h"

#define BADGE_LENGTH 6

void list_playlists();
void list_playlist_tracks(int pid);
int close_command(FILE* fp);

void free_playlists(finyl_playlist* pls, int size);

int get_playlists(finyl_playlist** pls, char* usb);

int get_playlist_tracks(finyl_track_meta** tms, char* usb, int pid);

int get_track(finyl_track* t, char* usb, int tid);

int get_all_tracks(finyl_track** ts, char* usb);
