#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <thread>
#include "finyl.h"
#include "rekordbox.h"
#include "dev.h"
#include "controller.h"
#include "extern.h"


int main(int argc, char **argv) {
  if (argc < 2) {
    printf("usage ./finyl <path to rekordbox usb: example /media/null/22BC-F655/ > [<soundcard name (default is \"default\")> <period_size (default is 1024)> <fps (default 30)]\n inside [] are optional\n");
    return 0;
  }
  std::string usbroot = argv[1];
  plug(usbroot);
  device = "default";
  sample_rate = 44100;
  snd_pcm_t* handle;
  period_size = 1024;
  if (argc >= 3) {
    device = argv[2];
  }
  if (argc >= 4) {
    period_size = std::stoi(argv[3]);
  }
  if (argc >=5) {
    fps = std::stoi(argv[4]);
  }
  
  snd_pcm_uframes_t buffer_size = period_size * 2;

  
  printf("want: %d %d\n", (int)buffer_size, (int)period_size);
  
  finyl_setup_alsa(&handle, &buffer_size, &period_size);
  period_size_2 = period_size*2;
  printf("buffer_size %ld, period_size %ld\n", buffer_size, period_size);
  
  auto th = std::thread(controller);
  
  finyl_run(NULL, NULL, NULL, NULL, handle);
  
  th.join();
}
