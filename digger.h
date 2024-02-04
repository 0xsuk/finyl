#ifndef DIGGER_H
#define DIGGER_H


#include "finyl.h"
#include <vector>
#include <memory>
#include <string_view>
#include "error.h"

void list_playlists();
void list_playlist_tracks(int pid);
error close_command(FILE* fp);

error get_playlists(std::vector<finyl_playlist>& pls, std::string_view usb);

error get_track(finyl_track& t, std::string_view usb, int tid);

error get_playlist_tracks(std::vector<finyl_track_meta>& tms, std::string_view usb, int pid);


#endif
