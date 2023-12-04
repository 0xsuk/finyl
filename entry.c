#include <stdio.h>
#include "finyl.h"
#include <alsa/asoundlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>


char* usb = "/media/null/22BC-F655/";
/* sysdefault:CARD=XDJRX */
/* char* device = "sysdefault:CARD=PCH"; */

void* thread(finyl_track* t) {
  /* sleep(5); */
  printf("testing thread\n");
  /* t->playing = false; */
  pthread_exit("ret");
}

int main() {
  snd_pcm_t* handle;
  snd_pcm_uframes_t buffer_size = 1024 * 2;
  snd_pcm_uframes_t period_size = 1024;
  finyl_setup_alsa(&handle, &buffer_size, &period_size);
  printf("buffer_size %ld, period_size %ld\n", buffer_size, period_size);
  
  finyl_track t;
  finyl_init_track(&t);
  finyl_set_process_callback(finyl_handle_play, finyl_a);
  finyl_set_process_callback(finyl_handle_master, finyl_all);

  char* files[2];
  files[0] = "birth.mp3";
  /* files[1] = "no_vocals.wav"; */
  if (finyl_read_channels_from_files(files, 1, &t) == -1) {
    printf("you failed\n");
    return -1;
  }
  
  finyl_print_track(&t);
  
  t.playing = true;
  
  pthread_t th;
  pthread_create(&th, NULL, thread, &t);
  
  finyl_run(&t, NULL, NULL, NULL, handle, buffer_size, period_size);

}
