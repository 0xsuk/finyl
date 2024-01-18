#include "cJSON.h"
#include "finyl.h"
#include <vector>
#include <memory>
#include <string_view>

void list_playlists();
void list_playlist_tracks(int pid);
int close_command(FILE* fp);

int get_playlists(std::vector<finyl_playlist>& pls, std::string_view usb);

int get_playlist_tracks(std::vector<finyl_track_meta>& tms, std::string_view usb, int pid);

int get_track(finyl_track& t, std::string_view usb, int tid);
