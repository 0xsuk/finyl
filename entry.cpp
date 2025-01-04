#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <thread>
#include "interface.h"

int main(int argc, char **argv) {
  srand (static_cast <unsigned> (time(0)));

  if (argc < 1) {
    printf("usage ./finyl [<soundcard name (default is \"default\")> <period_size (default is 128)> <fps (default 30)]\n inside [] are optional\n");
    return 0;
  }
  char* device = "default";
  
  if (argc >= 2) {
    device = argv[1];
  }
  
  if (argc >= 3) {
    int period_size = std::stoi(argv[2]);
    gApp.audio->set_period_size(period_size);
  }
  
  if (argc >=4) {
    int fps = std::stoi(argv[3]);
    gApp.interface->set_fps(fps);
  }
  
  
  gApp.audio->setup_alsa(device);
  
  gApp.run();
}
