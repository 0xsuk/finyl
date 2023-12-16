#include "dev.h"

#ifndef USB
#define USB "/media/null/22BC-F655/"
#endif
#define STR_IMPL(x) #x
#define STR(x) STR_IMPL(x)
char* usb = STR(USB);

char finyl_output_path[256];

char* get_finyl_output_path() {
  char* home = getenv("HOME");
  snprintf(finyl_output_path, sizeof(finyl_output_path), "%s/.finyl-output", home);
  return finyl_output_path;
}

void print_track_meta(finyl_track_meta* tm) {
  printf("id is %d\n", tm->id);
  printf("key is %d\n", tm->musickeyid);
  printf("bpm is %d\n", tm->bpm);
  printf("title is %s\n", tm->title);
  printf("filepath is %s\n", tm->filepath);
}

void print_track(finyl_track* t) {
  printf("track {\n");
  /* print_track_meta(t->meta); */
  printf("\tid: %d\n", t->meta.id);
  printf("\tfilename: %s\n", t->meta.filename);
  printf("\tbpm: %d\n", t->meta.bpm);
  printf("\tnchunks: %d\n", t->nchunks);
  printf("\tlength: %d\n", t->length);
  printf("\tloop_in: %lf\n", t->loop_in);
  printf("\tloop_out: %lf\n", t->loop_out);
  printf("\tcues_size: %d\n", t->cues_size);
  printf("\tbeats_size: %d\n", t->beats_size);
  printf("\tindex: %lf\n", t->index);
  printf("\tspeed: %lf\n", t->speed);
  printf("\tchannels_size: %d\n", t->channels_size);
  printf("\tplaying: %d\n", t->playing);
  printf("}\n");
}
