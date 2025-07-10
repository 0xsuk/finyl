#include "midi.h"

int test_midi() {
  MidiLearn ml{};
  // int err = ml.open_device("hw:1,0");
  int err = ml.open_device("hw:2,0,0");
  if (err) {
    fprintf(stderr,"snd_rawmidi_open failed: %d\n", err);
    
  }
  printf("opened\n");
  ml.learn();

  return 0;
}

int main() {
  test_midi();
}
