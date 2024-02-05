#include "rekordbox.h"
#include "dev.h"
std::string root = "/media/null/71CD-A534/";

void test_getPlaylistsTrack() {
  rekordbox::Usb usb;
  usb.root = root;
  
  rekordbox::plug(usb);
  std::vector<finyl_track_meta> tms;
  rekordbox::getPlaylistTracks(tms, usb, 14);

  for (auto& tm: tms) {
    print_track_meta(tm);
  }
}

void test_getTrack() {
  finyl_track t;

  rekordbox::getTrack(t, "/media/null/71CD-A534/PIONEER/USBANLZ/P074/00012A52/ANLZ0000.DAT");
}

int main() {
  test_getTrack();
}
