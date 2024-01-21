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
  printf("\tlength: %d\n", t.length);
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

bool is_chunk_same(finyl_chunk& c1, finyl_chunk& c2) {
  if (c1.size() != c2.size()) return false;
  for (int i = 0; i<c1.size(); i++) {
    finyl_sample s1 = c1[i];
    finyl_sample s2 = c2[i];

    if (s1 != s2) return false;
  }

  return true;
}

bool is_stem_same(finyl_stem& s1, finyl_stem& s2) {
  for (int i = 0; i<s1.size(); i++) {
    finyl_chunk& c1 = s1[i];
    finyl_chunk& c2 = s2[i];
    
    if (!is_chunk_same(c1, c2)) return false;
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
