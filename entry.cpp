#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>
#include "finyl.h"
#include "dev.h"
#include "controller.h"


int main(int argc, char **argv) {
  if (argc < 2) {
    printf("usage ./finyl <path to rekordbox usb: example /media/null/22BC-F655/ > [<soundcard name (default is \"default\")> <period_size (default is 1024)>]\n inside [] are optional\n");
    return 0;
  }
  std::string usb = argv[1];
  std::string device = "default";
  snd_pcm_t* handle;
  snd_pcm_uframes_t buffer_size = 1024 * 2;
  snd_pcm_uframes_t period_size = 1024;
  if (argc >= 3) {
    device = argv[2];
  }
  if (argc >= 4) {
    period_size = std::stoi(argv[3]);
  }
  
  init_globals(usb, device, period_size);
  finyl_setup_alsa(&handle, &buffer_size, &period_size);
  printf("buffer_size %ld, period_size %ld\n", buffer_size, period_size);
  
  pthread_t c;
  if (pthread_create(&c, NULL, controller, NULL) != 0) {
    perror("pthread_create");
    return 1;
  }
  
  finyl_run(NULL, NULL, NULL, NULL, handle);
  
  pthread_join(c, NULL);
}
