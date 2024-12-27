#include "midi.h"
#include "controller.h"
#include <thread>

MidiLearn::MidiLearn(): handle_in(0) {
  reset();
}

void MidiLearn::reset() {
  stop = false;
  status = '\0';
  control = -1;
  max = -1;
  min = -1;
}

int MidiLearn::open_device(char* _device_in) {
  device_in = _device_in;
  return snd_rawmidi_open(&handle_in, NULL, device_in, 0);
}

void MidiLearn::memo(int _val) {
  val = _val;
  if (val > max) {
    max = val;
  } else if (val < min) {
    min = val;
  }
}

bool is_note(unsigned char status) {
  return status >= 0x90 && status <= 0x9f;
}

bool is_control(unsigned char status) {
  return status >= 0xb0 && status <= 0xbf;
}

bool is_learnable_status(unsigned char status) {  
  return is_note(status) || is_control(status);
}

void MidiLearn::print_info() {
  printf("\33[2K\rstatus:%02x control:%d val:%d max:%d min:%d", status, control, val, max, min);
  fflush(stdout);
}

void MidiLearn::save(char* action_name) {
  midiToActionMap.push_back({status, control, action_name});
}

void MidiLearn::print_map() {
  for (auto& ent: midiToActionMap) {
    printf("{0x%02x, %d, \"%s\"},\n", ent.status, ent.control, ent.action_name);
  }
}

void MidiLearn::listen() {
  while (!stop.load()) {
    ssize_t ret = snd_rawmidi_read(handle_in, buf, sizeof(buf));
    if (ret <= 0) {
      fprintf(stderr, "read error: %d - %s\n", (int)ret, snd_strerror(ret));
      return;
    }  
  
    for (int i=0; i<ret; i++) {
      if (status == '\0') {
        //keep seeking expected status
        if (is_learnable_status(buf[i])) {
          status = buf[i];
          control = buf[++i];
          max = buf[++i];
          min = buf[i];
          val = buf[i];
        }
        print_info();
        continue;
      }

      //i points to status
      //filter sequence based on status and control
      if (is_learnable_status(buf[i]) && buf[++i] == control) {
        //update memo
        memo(buf[++i]);
        print_info();
      } else {
        continue;
      }
    }
  }
}

void MidiLearn::stop_listening() {
  // pthread_cancel;
  stop.store(true);
}

MidiParser::MidiParser(): stop(false), handle_in(0) {
}

int MidiParser::open_device(char *_device_in) {
  device_in = _device_in;
  return snd_rawmidi_open(&handle_in, NULL, device_in, 0);
}

void MidiParser::handle(std::function<void (int, unsigned char *)> handler) {
  while (!stop) {
    ssize_t ret = snd_rawmidi_read(handle_in, buf, sizeof(buf));
    if (ret <= 0) {
      fprintf(stderr, "read error: %d - %s\n", (int)ret, snd_strerror(ret));
      continue;
    }

    for (int i=0; i<ret; i++) {
      if (is_learnable_status(buf[i]) && i+2<ret) {
        //TODO 3
        handler(3, &buf[i]);
        i=i+2;
      }
    }
  }
}

void MidiLearn::learn() {
  for (int i = 0; i<gApp.controller->actionToFuncMap.size(); i++) {
    auto& ent = gApp.controller->actionToFuncMap[i];
    printf("\n%s:\n", ent.base_action);
    int d;
    auto t = std::thread([&](){listen();}); //NOTE: this
    scanf("%d", &d);
    stop_listening();
    t.join();
    printf("joined\n");

    if (d == 1) {
      i--;
    } else {
      save(ent.base_action);
    }
    reset();
  }

  print_map();
}

int test_midi() {
  MidiLearn ml{};
  int err = ml.open_device("hw:CARD=DDJ400");
  // int err = ml.open_device("hw:2,0,0");
  if (err) {
    fprintf(stderr,"snd_rawmidi_open failed: %d\n", err);
  }
  printf("opened\n");
  ml.learn();

  return 0;
}
