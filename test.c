#include <stdio.h>
#include "finyl.h"
#include <alsa/asoundlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>


char* usb = "/media/null/22BC-F655/";
/* sysdefault:CARD=XDJRX */
/* char* device = "sysdefault:CARD=PCH"; */

int main() {
  snd_pcm_t* handle;
  snd_pcm_uframes_t period_size = 1024*4;
  snd_pcm_uframes_t buffer_size = period_size * 2;
  finyl_setup_alsa(&handle, &buffer_size, &period_size);
  printf("buffer_size %ld, period_size %ld\n", buffer_size, period_size);
  
  finyl_track t;
  finyl_init_track(&t);

  char* files[2];
  files[0] = "birth_voc.wav";
  files[1] = "birth_novoc.wav";
  if (finyl_read_channels_from_files(files, 2, &t) == -1) {
    printf("you failed\n");
    return -1;
  }
  
  finyl_print_track(&t);
  
  t.playing = true;
  finyl_run(&t, NULL, NULL, NULL, handle, buffer_size, period_size);
}
