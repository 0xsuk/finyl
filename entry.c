#include <alsa/asoundlib.h>
#include <math.h>
#include <stdbool.h>
#include "finyl.h"

int main(int argc, char** argv) {
  if (argc != 2) {
    printf("Usage: ./finyl filename\n");
    exit(0);
  }
  track t;
  init_track(&t);
  snd_pcm_t* handle;
  snd_pcm_uframes_t buffer_size = 1024 * 2;
  snd_pcm_uframes_t period_size = 1024;
  setup_alsa(&handle, &buffer_size, &period_size);
  printf("buffer_size %ld, period_size %ld\n", buffer_size, period_size);
  if (load_track(argv[1], &t) == -1) {
    return -1;
  }
  run(&t, handle, period_size);
  return 0;
}
