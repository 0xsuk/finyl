#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>
#include "finyl.h"
#include "dev.h"
#include "controller.h"

int main(int argc, char **argv) {
  
  snd_pcm_t* handle;
  snd_pcm_uframes_t buffer_size = 1024 * 2;
  snd_pcm_uframes_t period_size = 1024;
  finyl_setup_alsa(&handle, &buffer_size, &period_size);
  printf("buffer_size %ld, period_size %ld\n", buffer_size, period_size);
  
  pthread_t key_thread;
  if (pthread_create(&key_thread, NULL, key_input, NULL) != 0) {
    perror("pthread_create");
    return 1;
  }
  
  finyl_run(NULL, NULL, NULL, NULL, handle, buffer_size, period_size);

  pthread_join(key_thread, NULL);
}
