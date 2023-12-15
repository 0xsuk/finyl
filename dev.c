#include "finyl.h"

char* usb = "/media/null/22BC-F655/";

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
  printf("\tid: %d\n", t->meta->id);
  printf("\tfilename: %s\n", t->meta->filename);
  printf("\tbpm: %d\n", t->meta->bpm);
  printf("\tnchunks: %d\n", t->nchunks);
  printf("\tlength: %d\n", t->length);
  printf("\tcues_size: %d\n", t->cues_size);
  printf("\tbeats_size: %d\n", t->beats_size);
  printf("\tindex: %lf\n", t->index);
  printf("\tspeed: %lf\n", t->speed);
  printf("\tchannels_size: %d\n", t->channels_size);
  printf("\tplaying: %d\n", t->playing);
  printf("}\n");
}
