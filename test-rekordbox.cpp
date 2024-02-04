#include "rekordbox.h"
#include "dev.h"

int main() {
  std::string root = "/media/null/71CD-A534/";
  rekordbox::Usb usb;
  usb.root = root;
  
  rekordbox::plug(usb);
  std::vector<finyl_track_meta> tms;
  rekordbox::getPlaylistTracks(tms, usb, 14);

  for (auto& tm: tms) {
    print_track_meta(tm);
  }
}
