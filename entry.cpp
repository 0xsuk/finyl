#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <thread>
#include "interface.h"

int main(int argc, char **argv) {
  srand (static_cast <unsigned> (time(0)));

  if (argc < 2) {
    printf("usage ./finyl <path to rekordbox usb: example /media/null/22BC-F655/ > [<soundcard name (default is \"default\")> <period_size (default is 128)> <fps (default 30)]\n inside [] are optional\n");
    return 0;
  }
  std::string usbroot = argv[1];
  int err = plug(usbroot);
  if (err == 1) {
    return 1;
  }
  char* device = "default";
  
  if (argc >= 3) {
    device = argv[2];
  }
  
  if (argc >= 4) {
    int period_size = std::stoi(argv[3]);
    gApp.audio->set_period_size(period_size);
  }
  
  if (argc >=5) {
    int fps = std::stoi(argv[4]);
    gApp.interface->set_fps(fps);
  }
  
  
  gApp.audio->setup_alsa(device);
  
  gApp.run();
}
