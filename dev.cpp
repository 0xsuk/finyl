#include "dev.h"

void print_track_meta(finyl_track_meta& tm) {
  printf("id is %d\n", tm.id);
  printf("key is %d\n", tm.musickeyid);
  printf("bpm is %d\n", tm.bpm);
  printf("title is %s\n", tm.title.c_str());
  printf("filepath is %s\n", tm.filepath.c_str());
  for (int i = 0; i<tm.stem_filepaths.size(); i++) {
    printf("%dth channel filepath is %s\n", i, tm.stem_filepaths[i].c_str());
  }
}

void print_track(finyl_track& t) {
  printf("track {\n");
  /* print_track_meta(t->meta); */
  printf("\tid: %d\n", t.meta.id);
  printf("\tfilename: %s\n", t.meta.filename.c_str());
  printf("\tbpm: %d\n", t.meta.bpm);
  printf("\teffective bpm: %lf\n", t.meta.bpm * t.speed);
  printf("\tmsize: %d\n", t.msize);
  printf("\tloop_active: %d\n", t.loop_active);
  printf("\tloop_in: %lf\n", t.loop_in);
  printf("\tloop_out: %lf\n", t.loop_out);
  printf("\tindex: %lf\n", t.index);
  printf("\tspeed: %lf\n", t.speed);
  printf("\tstem_size: %d\n", (int)t.stems_size);
  printf("\tbeats_size:%d\n", (int)t.beats.size());
  printf("\tplaying: %d\n", t.playing);
  printf("}\n");
}

bool is_stem_same(finyl_stem& st1, finyl_stem& st2) {
  for (int i = 0; i<st1.ssize(); i++) {
    auto s1 = st1[i];
    auto s2 = st2[i];
    if (s1 != s2) return false;
    
  }

  return true;
}

void print_is_stem_same(finyl_stem& s1, finyl_stem& s2) {
  if (is_stem_same(s1, s2)) {
    printf("stem is same\n");
  } else {
    printf("stem is not same\n");
  }
}

void report(finyl_buffer& buffer) {
  int clip_count = 0;
  for (int i = 0; i<buffer.size(); i++) {
    finyl_sample a = buffer[i];
    if (a == 32767 || a == -32768) {
      clip_count++;
    }
  }

  printf("cc %d\n", clip_count);
}
