#include "dev.h"

#ifndef USB
#define USB "/media/null/22BC-F655/"
#endif
#define STR_IMPL(x) #x
#define STR(x) STR_IMPL(x)
std::string usb = STR(USB);

std::string get_finyl_output_path() {
  std::string home = getenv("HOME");
  std::string s = home + "/.finyl-output";
  return s;
}

void print_track_meta(finyl_track_meta& tm) {
  printf("id is %d\n", tm.id);
  printf("key is %d\n", tm.musickeyid);
  printf("bpm is %d\n", tm.bpm);
  printf("title is %s\n", tm.title.c_str());
  printf("filepath is %s\n", tm.filepath.c_str());
  for (int i = 0; i<tm.channel_filepaths.size(); i++) {
    printf("%dth channel filepath is %s\n", i, tm.channel_filepaths[i].c_str());
  }
}

void print_track(finyl_track& t) {
  printf("track {\n");
  /* print_track_meta(t->meta); */
  printf("\tid: %d\n", t.meta.id);
  printf("\tfilename: %s\n", t.meta.filename.c_str());
  printf("\tbpm: %d\n", t.meta.bpm);
  printf("\teffective bpm: %lf\n", t.meta.bpm * t.speed);
  printf("\tlength: %d\n", t.length);
  printf("\tloop_active: %d\n", t.loop_active);
  printf("\tloop_in: %lf\n", t.loop_in);
  printf("\tloop_out: %lf\n", t.loop_out);
  printf("\tindex: %lf\n", t.index);
  printf("\tspeed: %lf\n", t.speed);
  printf("\tchannels_size: %d\n", (int)t.channels.size());
  printf("\tbeats_size:%d\n", (int)t.beats.size());
  printf("\tplaying: %d\n", t.playing);
  printf("}\n");
}
