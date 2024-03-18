#include "rekordbox.h"
#include "extern.h"
#include "dev.h"
std::string root = "/run/media/null/71CD-A534/";

void test_getPlaylistsTrack() {
  std::vector<finyl_track_meta> tms;
  getPlaylistTrackMetas(tms, usbs[0], 14);

  for (auto& tm: tms) {
    print_track_meta(tm);
  }
}

void test_getTrack() {
  finyl_track t;

  readAnlz(usbs[0], t, 42);

  printf("cues size:%d\n", (int)t.cues.size());

  for (auto c: t.cues) {
    printf("cues: %d\n", c.time);
  }
}

int main() {
  plug(root);
  test_getTrack();
}
