#ifndef MIDI_H
#define MIDI_H

#include <alsa/asoundlib.h>
#include <functional>
#include <atomic>

struct MidiToAction {
  unsigned char status;
  unsigned char control;
  char* action_name;
};

class MidiLearn {
public:
  //the first byte of the sequence
  unsigned char status;
  //the second (note, control number)
  unsigned char control;
  //max min of the third value
  int max;
  int min;
  int val;
  
  MidiLearn();
  
  //0 on success
  int open_device(char* _device_in);

  void learn();

private:
  //listen midi sequences for one function
  void save(char* action_name);
  void listen();
  void stop_listening();
  
  void print_info();
  void reset();
  void memo(int _val);
  void print_map();

  std::atomic<bool> stop = false;
  
  snd_rawmidi_t *handle_in; 
  char* device_in;
  unsigned char buf[1024];

  std::vector<MidiToAction> midiToActionMap;
};


class MidiParser {
public:
  std::atomic<bool> stop;
  MidiParser();
  int open_device(char* _device_in);
  void handle(std::function<void(int len, unsigned char buf[])>);

// private:
  snd_rawmidi_t *handle_in; 
  char* device_in;
  unsigned char buf[1024];
};

int test_midi();
bool is_note(unsigned char status);
bool is_control(unsigned char status);
#endif
