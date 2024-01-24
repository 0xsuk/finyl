#include "finyl.h"
#include <vector>
#include <memory>
#include <string_view>
#include "error.h"

enum class DIG_ERR {
  BADGE_NOT_VALID,
  JSON_FAILED,
  CANT_OPEN_COMMAND,
  CANT_CLOSE_COMMAND,
  COMMAND_FAILED,
};

void list_playlists();
void list_playlist_tracks(int pid);
error<DIG_ERR> close_command(FILE* fp);

error<DIG_ERR> get_playlists(std::vector<finyl_playlist>& pls, std::string_view usb);

error<DIG_ERR> get_track(finyl_track& t, std::string_view usb, int tid);

error<DIG_ERR> get_track(finyl_track& t, std::string_view usb, int tid);

error<DIG_ERR> get_playlist_tracks(std::vector<finyl_track_meta>& tms, std::string_view usb, int pid);
