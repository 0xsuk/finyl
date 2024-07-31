#include <sys/resource.h>
#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include "finyl.h"
#include "util.h"
#include "dsp.h"
#include "dev.h"
#include "interface.h"
#include <memory>
#include <thread>
#include "rekordbox.h"
#include "extern.h"
#include "midi.h"
#include "controller.h"

#define ifvelocity(exp) if (velocity!=0) {exp}

//val and velocity is 0 to 1
ActionToFunc actionToFuncMap[] = {
  {"DeckA-gain", [](double val){set_gain(adeck, val);}},
  {"DeckA-gain0", [](double val){set_gain0(adeck, val);}},
  {"DeckA-gain1", [](double val){set_gain1(adeck, val);}},
  {"DeckA-gain0_1", [](double val){set_gain0_1(adeck, val);}},
  {"DeckA-eqlow", [](double val){set_bqGainLow(adeck, val);}},
  {"DeckA-delay", [](double val){delay(adeck, val);}},
  {"DeckA-press_cue", [](double velocity){press_cue_velocity(adeck, velocity);}},
  {"DeckA-toggle_playing", [](double velocity){toggle_playing_velocity(adeck, velocity);}},
  {"DeckA-inc_index", [](double velocity){inc_index(adeck, velocity);}},
  {"DeckA-dec_index", [](double velocity){dec_index(adeck, velocity);}},
  {"DeckA-inc_delta_index", [](double velocity){inc_delta_index(adeck, velocity);}},
  {"DeckA-dec_delta_index", [](double velocity){dec_delta_index(adeck, velocity);}},
  {"DeckA-loop_in", [](double velocity){ifvelocity(loop_in_now(adeck);)}},
  {"DeckA-loop_out", [](double velocity){ifvelocity(loop_out_now(adeck);)}},
  {"DeckA-loop_deactivate", [](double velocity){ifvelocity(loop_deactivate(adeck);)}},
  {"DeckA-sync_bpm", [](double velocity){ifvelocity(sync_bpm(adeck);)}},
  {"DeckA-inc_speed", [](double velocity){ifvelocity(inc_speed(adeck);)}},
  {"DeckA-dec_speed", [](double velocity){ifvelocity(dec_speed(adeck);)}},
  {"DeckA-toggle_delay", [](double velocity){ifvelocity(toggle_delay(adeck);)}},
  {"DeckA-toggle_mute_gain0", [](double velocity){ifvelocity(toggle_mute0(adeck);)}},
  {"DeckA-toggle_master", [](double velocity){ifvelocity(toggle_master(adeck);)}},


  {"DeckB-gain", [](double val){set_gain(bdeck, val);}},
  {"DeckB-gain0", [](double val){set_gain0(bdeck, val);}},
  {"DeckB-gain1", [](double val){set_gain1(bdeck, val);}},
  {"DeckB-gain0_1", [](double val){set_gain0_1(bdeck, val);}},
  {"DeckB-eqlow", [](double val){set_bqGainLow(bdeck, val);}},
  {"DeckB-delay", [](double val){delay(bdeck, val);}},
  {"DeckB-press_cue", [](double velocity){press_cue_velocity(bdeck, velocity);}},
  {"DeckB-toggle_playing", [](double velocity){toggle_playing_velocity(bdeck, velocity);}},
  {"DeckB-inc_index", [](double velocity){inc_index(bdeck, velocity);}},
  {"DeckB-dec_index", [](double velocity){dec_index(bdeck, velocity);}},
  {"DeckB-inc_delta_index", [](double velocity){inc_delta_index(bdeck, velocity);}},
  {"DeckB-dec_delta_index", [](double velocity){dec_delta_index(bdeck, velocity);}},
  {"DeckB-loop_in", [](double velocity){ifvelocity(loop_in_now(bdeck);)}},
  {"DeckB-loop_out", [](double velocity){ifvelocity(loop_out_now(bdeck);)}},
  {"DeckB-loop_deactivate", [](double velocity){ifvelocity(loop_deactivate(bdeck);)}},
  {"DeckB-sync_bpm", [](double velocity){ifvelocity(sync_bpm(bdeck);)}},
  {"DeckB-inc_speed", [](double velocity){ifvelocity(inc_speed(bdeck);)}},
  {"DeckB-dec_speed", [](double velocity){ifvelocity(dec_speed(bdeck);)}},
  {"DeckB-toggle_delay", [](double velocity){ifvelocity(toggle_delay(bdeck);)}},
  {"DeckB-toggle_mute_gain0", [](double velocity){ifvelocity(toggle_mute0(bdeck);)}},

  {"inc_wave_range", [](double velocity){ifvelocity(set_wave_range(interface, interface.wave_range*2);)}},
  {"dec_wave_range", [](double velocity){ifvelocity(set_wave_range(interface, interface.wave_range/2);)}},
};

const int len_actionToFuncMap = sizeof(actionToFuncMap)/sizeof(actionToFuncMap[0]);


MidiToAction xdj_xz[] = {
  {0xb4, 17, "DeckA-gain"},
  {0xb4, 1, "DeckA-gain0_1"},
  {0xb4, 5, "DeckA-delay"},
  {0xb4, 4, "DeckA-eqlow"},
  {0x90, 1, "DeckA-press_cue"},
  {0x90, 0, "DeckA-toggle_playing"},
  {0x95, 1, "DeckA-inc_index"},
  {0x95, 0, "DeckA-dec_index"},
  {0x95, 5, "DeckA-inc_delta_index"},
  {0x95, 4, "DeckA-dec_delta_index"},
  {0x90, 6, "DeckA-loop_in"},
  {0x90, 7, "DeckA-loop_out"},
  {0x90, 8, "DeckA-loop_deactivate"},
  {0x90, 30, "DeckA-sync_bpm"},
  {0x95, 3, "DeckA-inc_speed"},
  {0x95, 2, "DeckA-dec_speed"},
  {0xb4, 72, "DeckA-toggle_mute_gain0"},
  {0xb4, 70, "DeckA-toggle_delay"},
  {0x90, 17, "DeckA-toggle_master"},

  {0xb4, 18, "DeckB-gain"},
  {0xb4, 6, "DeckB-gain0_1"},
  {0xb4, 10, "DeckB-delay"},
  {0xb4, 9, "DeckB-eqlow"},
  {0x91, 1, "DeckB-press_cue"},
  {0x91, 0, "DeckB-toggle_playing"},
  {0x96, 1, "DeckB-inc_index"},
  {0x96, 0, "DeckB-dec_index"},
  {0x96, 5, "DeckB-inc_delta_index"},
  {0x96, 4, "DeckB-dec_delta_index"},
  {0x91, 6, "DeckB-loop_in"},
  {0x91, 7, "DeckB-loop_out"},
  {0x91, 8, "DeckB-loop_deactivate"},
  {0x91, 30, "DeckB-sync_bpm"},
  {0x96, 3, "DeckB-inc_speed"},
  {0x96, 2, "DeckB-dec_speed"},
  {0xb4, 73, "DeckB-toggle_mute_gain0"},
  {0xb4, 71, "DeckB-toggle_delay"},

  {0xb4, 107, "inc_wave_range"},
  {0xb4, 105, "dec_wave_range"},
};

// MidiToAction launchkey[] = {
//   {0xb0, 21, "DeckA-gain"},
//   {0xb0, 22, "DeckA-gain0_1"},
//   {0xb0, 23, "DeckA-delay"},
//   {0xb0, 24, "DeckA-eqlow"},
//   {0x99, 40, "DeckA-press_cue"},
//   {0x99, 36, "DeckA-toggle_playing"},
//   {0x90, 53, "DeckA-inc_index"},
//   {0x90, 52, "DeckA-dec_index"},
//   {0x90, 50, "DeckA-inc_delta_index"},
//   {0x90, 48, "DeckA-dec_delta_index"},
//   {0x99, 41, "DeckA-loop_in"},
//   {0x99, 42, "DeckA-loop_out"},
//   {0x99, 43, "DeckA-loop_deactivate"},
//   {0x90, 59, "DeckA-sync_bpm"},
//   {0x90, 57, "DeckA-inc_speed"},
//   {0x90, 55, "DeckA-dec_speed"},
//   {0x99, 37, "DeckA-toggle_mute_gain0"},
//   {0x99, 38, "DeckA-toggle_delay"},
//   {0x90, 49, "DeckA-toggle_master"},

//   {0xb0, 25, "DeckB-gain"},
//   {0xb0, 26, "DeckB-gain0_1"},
//   {0xb0, 27, "DeckB-delay"},
//   {0xb0, 28, "DeckB-eqlow"},
//   {0x99, 48, "DeckB-press_cue"},
//   {0x99, 44, "DeckB-toggle_playing"},
//   {0x90, 65, "DeckB-inc_index"},
//   {0x90, 64, "DeckB-dec_index"},
//   {0x90, 62, "DeckB-inc_delta_index"},
//   {0x90, 60, "DeckB-dec_delta_index"},
//   {0x99, 49, "DeckB-loop_in"},
//   {0x99, 50, "DeckB-loop_out"},
//   {0x99, 51, "DeckB-loop_deactivate"},
//   {0x90, 71, "DeckB-sync_bpm"},
//   {0x90, 69, "DeckB-inc_speed"},
//   {0x90, 67, "DeckB-dec_speed"},
//   {0x99, 45, "DeckB-toggle_mute_gain0"},
//   {0x99, 46, "DeckB-toggle_delay"},

//   {0xbf, 104, "inc_wave_range"},
//   {0xbf, 105, "dec_wave_range"},
// };

void midi_handler(int len, unsigned char buf[]) {
  if (len < 3) {
    return; //TODO for now
  }

  //search action name by buf[0:1] from midilearn file
  //search func by action name from actionToFunc
  //call it
  
  for (auto& ent: xdj_xz) {
    if (ent.status == buf[0] && ent.control == buf[1]) {
      for (auto& atf: actionToFuncMap) {
        if (atf.base_action == ent.action_name) {
          //every actions should be quick or non blocking
          atf.func(buf[2]/127.0);
        }
      }
      
      break;
    }
  }
  
}

void slide_right(finyl_track* t) {
  double backup = t->get_speed();
  double y = 0;
  double x = 0;
  while (y>=0) {
    y  = -x*(x-2);
    t->set_speed(y);
    x = 0.01 + x;
    usleep(1000);
    printf("t is %lf\n", t->get_speed());
  }
  t->set_speed(backup);
}

//n = 0 means load an original file
void load_track_nstems(finyl_track** dest, int tid, finyl_deck_type deck, int n) {
  finyl_track* before = *dest;
  
  auto t = std::make_unique<finyl_track>();
  getTrackMeta(t->meta, usbs[0], tid);
  readAnlz(usbs[0], *t, tid);
  
  printf("file: %s\n", t->meta.filename.data());
  
  if (t->meta.stem_filepaths.size() < n) {
    printf("not enough stems\n");
    return;
  }
  
  
  std::vector<std::string> filepaths;
  filepaths.resize(max(n, 1));
  if (n > 0) {
    for (int i = 0; i<min((int)t->meta.stem_filepaths.size(), n); i++) {
      filepaths[i] = t->meta.stem_filepaths[i];
    }
  } else {
    filepaths[0] = t->meta.filepath;
  }
  
  if (auto err = finyl_read_stems_from_files(filepaths, *t)) {
    print_err(err);
    return;
  }
  
  printf("%s\n", t->meta.filename.data());

  *dest = t.release();
  if (deck == finyl_a) {
    interface.render_adeck = true;
  } else if (deck == finyl_b) {
    interface.render_bdeck = true;
  }

  if (before != nullptr) {
    add_track_to_free(before);
  }
}

void load_track(finyl_track** dest, int tid, finyl_deck_type deck) {
  load_track_nstems(dest, tid, deck, 0);
}

// void magic(int port, struct termios* tty) {
//   //reads existing settings
//   if(tcgetattr(port, tty) != 0) {
//     printf("Error %i from tcgetattr: %s\n", errno, strerror(errno));
//     return;
//   }

//   cfsetispeed(tty, B9600);
//   cfsetospeed(tty, B9600);
  
//   tty->c_cflag &= ~PARENB; // Clear parity bit
//   tty->c_cflag &= ~CSTOPB; // Clear stop field
//   tty->c_cflag &= ~CSIZE; // Clear all bits that set the data size 
//   tty->c_cflag |= CS8; // 8 bits per byte
//   tty->c_cflag &= ~CRTSCTS; // Disable RTS/CTS hardware flow control
//   tty->c_cflag |= CREAD | CLOCAL; // Turn on READ & ignore ctrl lines

//   tty->c_lflag &= ~ICANON;
//   tty->c_lflag &= ~ECHO; // Disable echo
//   tty->c_lflag &= ~ECHOE; // Disable erasure
//   tty->c_lflag &= ~ECHONL; // Disable new-line echo
//   tty->c_lflag &= ~ISIG; // Disable interpretation of INTR, QUIT and SUSP
//   tty->c_iflag &= ~(IXON | IXOFF | IXANY); // Turn off s/w flow ctrl
//   tty->c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL); // Disable any special handling of received bytes

//   tty->c_oflag &= ~OPOST; // Prevent special interpretation of output bytes (e.g. newline chars)
//   tty->c_oflag &= ~ONLCR; // Prevent conversion of newline to carriage return/line feed

//   tty->c_cc[VTIME] = 10;    // Wait for up to 1s (10 deciseconds), returning as soon as any data is received.
//   tty->c_cc[VMIN] = 0;
// }

// static bool match(char* s, char* x, int i) {
//   return (strlen(x) == i && strncmp(s, x, i) == 0);
// }

// static std::unique_ptr<char[]> trim_value(char* s, int i) {
//   int len = strlen(s);
//   int vlen = len - i - 1;
//   auto v = std::make_unique<char[]>(vlen+1);
//   strncpy(v.get(), &s[i+1], vlen);
//   v[vlen] = '\0';
//   return v;
// }

// static bool is_on(char* v) {
//   if (strcmp(v, "on") == 0) {
//     return true;
//   }
//   return false;
// }

// void handle_delay_on(char* v, Delay& d, finyl_track& deck) {
//   if (is_on(v)) {
//     if (d.on) {
//       d.on = false;
//       printf("off\n");
//     } else {
//       double bpm = (deck.meta.bpm/100.0)*deck.get_speed();
//       d.setMsize((sample_rate*60)/bpm*1.0);
//       d.on = true;
//       printf("on\n");
//     }
//   }
// }

// void handle_toggle_playing(Deck& deck, char* v) {
//   if (is_on(v)) {
//     toggle_playing(deck);
//   }
// }

// void handle_loop_in(char* v, finyl_track* t) {
//   if (is_on(v)) {
//     loop_in_now(*t);
//   }
// }

// void handle_loop_out(char* v, finyl_track* t) {
//   if (is_on(v)) {
//     loop_out_now(*t);
//   }
// }

// void handle_vi(char* v, double * vo, double* i, bool flip=false) { //vocal - inst
//   char* endptr;
//   int n = strtol(v, &endptr, 10);
//   if (endptr == v || *endptr != '\0') return;
//   if (flip) {
//     n = abs(n-4095);
//   }
//   if (n < 2046) {
//     *vo = 1.0;
//     *i = n / 2046.0;
//   } else {
//     *i = 1.0;
//     *vo = abs(n-4095.0) / 2046.0;
//   }
// }

// void handle_gain(char* v, double* g, bool flip=false) {
//   char* endptr;
//   int n = strtol(v, &endptr, 10);
//   if (endptr == v || *endptr != '\0') return;
//   if (flip) {
//     n = abs(n-4095);
//   }
//   *g = n / 4095.0;
// }

// void handle_what(char* s) {
//   int i = find_char_last(s, ':');
//   if (i<1) {
//     return;
//   }
  
//   auto _v = trim_value(s, i);
//   char* v = _v.get();

//   if (match(s, "pot1", i)) {
//     handle_gain(v, &adeck.gain, true);
//   } else if (match(s, "pot0", i)) {
//     handle_gain(v, &bdeck.gain, true);
//   } else if (match(s, "pot2", i)) {
//     handle_vi(v, &a0_gain, &a1_gain);
//   } else if (match(s, "pot3", i)) {
//     handle_vi(v, &b0_gain, &b1_gain);
//   }
//   if (adeck != NULL) {
//     if (match(s, "button3", i)) {
//       handle_toggle_playing(v, adeck);
//     }
//     else if (match(s, "button4", i)) {
//       handle_loop_in(v, adeck);
//     }
//     else if (match(s, "button10", i)) {
//       handle_loop_out(v, adeck);
//     } else if (match(s, "button8", i)) {
//       handle_delay_on(v, a_delay, *adeck);
//     }
//   }
  
//   if (bdeck != NULL) {
//     if (match(s, "button2", i)) {
//       handle_toggle_playing(v, bdeck);
//     }
//     else if (match(s, "button5", i)) { //bdeck
//       handle_loop_in(v, bdeck);
//     }
//     else if(match(s, "button6", i)) {
//       handle_loop_out(v, bdeck);
//     } else if (match(s, "button9", i)) {
//       handle_delay_on(v, b_delay, *bdeck);
//     }
//   }

//   if (adeck != NULL && bdeck!= NULL) {
//     if (match(s, "button7", i)) {
//       adeck->set_speed(bdeck->get_speed() * ((double)bdeck->meta.bpm / adeck->meta.bpm));
//       printf("synced adeck->speed: %lf\n", adeck->get_speed());
//     }
//     if (match(s, "button1", i)) {
//       bdeck->set_speed(adeck->get_speed() * ((double)adeck->meta.bpm / bdeck->meta.bpm));
//       printf("synced bdeck->speed: %lf\n", bdeck->get_speed());
//     }
//   }
// }

// void* serial(void* args) {
//   int serialPort = open("/dev/ttyUSB0", O_RDWR);

//   printf("serial\n");
  
//   if (serialPort < 0) {
//     printf("Error %i from serial open: %s\n", errno, strerror(errno));
//     return NULL;
//   }

//   struct termios tty;
//   magic(serialPort, &tty);
  
//   if (tcsetattr(serialPort, TCSANOW, &tty) != 0) {
//     printf("Error %i from tcsetattr: %s\n", errno, strerror(errno));
//     return NULL;
//   }
  
//   write(serialPort, "init", 4);
  
//   char tmp[100];
//   int tmplen = 0;
//   while(1) {
//     char read_buf [256]; //could contain error
//     int num_bytes = read(serialPort, &read_buf, sizeof(read_buf));
//     if (num_bytes < 0) {
//       printf("Error reading: %s\n", strerror(errno));
//       return NULL;
//     }
    
//     int start_i = 0;
//     for (int i = 0; i<num_bytes; i++) {
//       int linelen = i - start_i; //\n not counted
//       char c = read_buf[i];
//       if (c != '\n') {
//         continue;
//       }
//       if (tmplen > 0) {
//         int len = tmplen + linelen;
//         char s[len+1];
//         strncpy(s, tmp, tmplen);
//         s[tmplen] = '\0';
//         strncat(s, &read_buf[start_i], linelen);
//         handle_what(s);
//         tmplen = 0;
//       } else {
//         if (linelen == 0) continue;
//         //no need to care tmp
//         char s[linelen+1];
//         strncpy(s, &read_buf[start_i], linelen);
//         s[linelen] = '\0';
//         handle_what(s);
//       }

//       start_i = i+1; //i+1 is next to newline
//     }

//     tmplen = num_bytes-start_i;
//     strncpy(tmp, &read_buf[start_i], tmplen);
//   }
  
//   close(serialPort);
//   return NULL;
// }

// struct KeyHandler {
//   char key;
//   void* condition;
//   void* func;
//   void* args;
// };


// bool a_exist() {
  
// }

// bool b_exist() {
  
// }

// bool ab_exist() {
  
// }

// KeyHandler key_handlers[] = {
//   {'-', (void*)&a_exist, }
// }

void handle_key(char x) {
  if (adeck.pTrack != nullptr) {
    switch (x) {
    case '-': {
      set_bqGainLow(adeck, adeck.bqisoState->bqGainLow+0.1);
      return;
    }
    case '=': {
      set_bqGainLow(adeck, adeck.bqisoState->bqGainLow-0.1);
      return;
    }
    case 'z':
      toggle_delay(adeck);
      return;
    case 'g':
      toggle_playing(adeck);
      return;
    case 'G':
      set_speed(adeck, 1);
      return;
    case 'N':
      set_gain0(adeck, 0);
      return;
    case 'J':
      set_gain0(adeck, 1);
      return;
    case 'n':
      set_gain1(adeck, max(adeck.gain1-0.05, 0.0));
      return;
    case 'j':
      set_gain1(adeck, min(adeck.gain1+0.05, 1.0));
      return;
    case 'c':
      jump_cue(adeck);
      return;
    case 'a': {
      set_speed(adeck, adeck.pTrack->get_speed()+0.01);
      return;
    }
    case 's': {
      set_speed(adeck, adeck.pTrack->get_speed() - 0.01);
      return;
    }
    case 't': {
      add_active_cue(adeck);
      return;
    }
    case '1':
      /* mark loop in */
      loop_in_now(adeck);
      return;
    case '!':
      loop_deactivate(adeck);
      return;
    case '2': {
      loop_out_now(adeck);
      return;
    }
    case 'v':
      adeck.pTrack->set_index(adeck.pTrack->get_refindex() + 300);
      return;
    case 'V':
      adeck.pTrack->set_index(adeck.pTrack->get_refindex() - 300);
      return;
    case '5':
      adeck.pTrack->set_index(adeck.pTrack->get_refindex() + 3000);
      return;
    case '%':
      adeck.pTrack->set_index(adeck.pTrack->get_refindex() - 3000);
      return;

    }
  }
  
  if (bdeck.pTrack != nullptr) {
    switch (x) {
    case 'x':
      toggle_delay(bdeck);
      return;
    case 'h':
      toggle_playing(bdeck);
      return;
    case 'H':
      set_speed(bdeck, 1);
      return;
    case 'M':
      set_gain0(bdeck, 0);
      return;
    case 'K':
      set_gain0(bdeck, 1);
      return;
    case 'm':
      set_gain1(bdeck, max(bdeck.gain1-0.05, 0.0));
      return;
    case 'k':
      set_gain1(bdeck, min(bdeck.gain1+0.05, 1.0));
      return;
    case 'C':
      jump_cue(bdeck);
      return;
    case 'A':
      set_speed(bdeck, bdeck.pTrack->get_speed() + 0.01);
      return;
    case 'S':
      set_speed(bdeck, bdeck.pTrack->get_speed() - 0.01);
      return;
    case 'y': {
      add_active_cue(bdeck);
      return;
    }
    case '9':
      loop_in_now(bdeck);
      return;
    case '{':
      loop_deactivate(bdeck);
      return;
    case '0': {
      loop_out_now(bdeck);
      return;
    }
    case 'b':
      bdeck.pTrack->set_index(bdeck.pTrack->get_refindex() + 300);
      return;
    case 'B':
      bdeck.pTrack->set_index(bdeck.pTrack->get_refindex() - 300);
      return;
    case '6':
      bdeck.pTrack->set_index(bdeck.pTrack->get_refindex() + 3000);
      return;
    case '&':
      bdeck.pTrack->set_index(bdeck.pTrack->get_refindex() - 3000);
      return;

    }
  }
  
  if (adeck.pTrack != NULL && bdeck.pTrack != NULL) {
    switch (x) {
    case 'p':
      print_track(*adeck.pTrack);
      print_track(*bdeck.pTrack);
      return;
    case 'q': {
      set_speed(bdeck, adeck.pTrack->get_speed() * ((double)adeck.pTrack->meta.bpm / bdeck.pTrack->meta.bpm));
      return;
    }
    case 'Q': {
      set_speed(adeck, bdeck.pTrack->get_speed() * ((double)bdeck.pTrack->meta.bpm / adeck.pTrack->meta.bpm));
      return;
    }
    }
  }
  
  switch (x) {
  case 'X':
    finyl_running = false;
    break;
  case 'L':
    printPlaylists(usbs[0]);
    break;
  case 'l': {
    int pid;
    printf("pid:");
    scanf("%d", &pid);
    printf("listing...\n");
    printPlaylistTracks(usbs[0], pid);
    break;
  }
  case '4':
    free_tracks();
    break;
  case 'i': {
    int tid;
    printf("tid:");
    scanf("%d", &tid);
    printf("loading...%d\n", tid);
    load_track_nstems(&adeck.pTrack, tid, finyl_a, 2);
    break;
  }
  case 'o': {
    int tid;
    printf("tid:");
    scanf("%d", &tid);
    printf("loading...%d\n", tid);
    load_track_nstems(&bdeck.pTrack, tid, finyl_b, 2);
    break;
  }
  case 'I': {
    int tid;
    printf("tid:");
    scanf("%d", &tid);
    printf("loading...%d\n", tid);
    load_track(&adeck.pTrack, tid, finyl_a);
    break;
  }
  case 'O': {
    int tid;
    printf("tid:");
    scanf("%d", &tid);
    printf("loading...%d\n", tid);
    load_track(&bdeck.pTrack, tid, finyl_b);
    break;
  }
  // case '3':{
  //   std::thread(run_interface).detach();
  //   break;
  // }
  case '#': {
    free_tracks();
    break;
  }
  case '$': {
    memory_usage();
    break;
  }
  case '7': {
    set_wave_range(interface, interface.wave_range*2);
    break;
  }
  case '\'': {
    set_wave_range(interface, interface.wave_range/2);
    break;
  }
  }
}

void key_input() {
  static struct termios oldt, newt;
  
  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;
  newt.c_lflag &= ~(ICANON);          
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);
  while(finyl_running) {
    handle_key(getchar());                 
  }
  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
  printf("key_input closed\n");
  return;
}

void* controller() {
  std::thread(run_interface).detach();
  std::thread(key_input).detach();
  
  auto mp = MidiParser();
  int err = mp.open_device("hw:1,0,0");
  if (err) {
    return NULL;
  }
  std::thread([&]() {
    mp.handle(midi_handler);
  }).detach();
  
  auto mp2 = MidiParser();
  err = mp2.open_device("hw:2,0,0");
  if (err) {
    return NULL;
  }
  mp2.handle(midi_handler);
  return NULL;
}
