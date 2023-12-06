#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>
#include "finyl.h"
#include "controller.h"

char* finyl_output_path = "/home/null/.finyl-output"; //TODO

int main(int argc, char **argv) {
  
  snd_pcm_t* handle;
  snd_pcm_uframes_t buffer_size = 1024 * 2;
  snd_pcm_uframes_t period_size = 1024;
  finyl_setup_alsa(&handle, &buffer_size, &period_size);
  printf("buffer_size %ld, period_size %ld\n", buffer_size, period_size);
  
  finyl_track ta;
  finyl_init_track(&ta);
  adeck = &ta;
  ta.index = 44100 * 20;

  char* files_a[2];
  files_a[0] = "birth_voc.wav";
  files_a[1] = "birth_novoc.wav";
  if (finyl_read_channels_from_files(files_a, 2, &ta) == -1) {
    printf("you failed\n");
    return -1;
  }
  finyl_print_track(&ta);

  finyl_track tb;
  finyl_init_track(&tb);
  bdeck = &tb;
  tb.index = 44100 * 30;
  
  char* files_b[2];
  files_b[0] = "vocals.wav";
  files_b[1] = "no_vocals.wav";
  if (finyl_read_channels_from_files(files_b, 2, &tb) == -1) {
    printf("you failed\n");
    return -1;
  }
  finyl_print_track(&tb);
  
  pthread_t key_thread;
  if (pthread_create(&key_thread, NULL, key_input, NULL) != 0) {
    perror("pthread_create");
    return 1;
  }


  
  finyl_run(adeck, bdeck, NULL, NULL, handle, buffer_size, period_size);

  pthread_join(key_thread, NULL);
}
