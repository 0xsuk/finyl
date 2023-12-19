#include "interface.h"
#include "controller.h"

int main() {
  get_window_size();
  load_track(&adeck, 1);
  adeck->playing = true;
  interface(adeck);
}
