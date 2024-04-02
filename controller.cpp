#include <sys/resource.h>
#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include "finyl.h"
#include "util.h"
#include "dev.h"
#include "interface.h"
#include <pthread.h>
#include <memory>
#include "action.h"
#include "rekordbox.h"
#include "extern.h"

bool quantize = false;

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
void load_track_nchannels(finyl_track** dest, int tid, finyl_track_target deck, int n) {
  finyl_track* before = *dest;
  
  auto t = std::make_unique<finyl_track>();
  getTrackMeta(t->meta, usbs[0], tid);
  readAnlz(usbs[0], *t, tid);
  
  printf("file: %s\n", t->meta.filename.data());
  
  if (t->meta.stem_filepaths.size() < n) {
    printf("not enough channels\n");
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

void load_track(finyl_track** dest, int tid, finyl_track_target deck) {
  load_track_nchannels(dest, tid, deck, 0);
}

void magic(int port, struct termios* tty) {
  //reads existing settings
  if(tcgetattr(port, tty) != 0) {
    printf("Error %i from tcgetattr: %s\n", errno, strerror(errno));
    return;
  }

  cfsetispeed(tty, B9600);
  cfsetospeed(tty, B9600);
  
  tty->c_cflag &= ~PARENB; // Clear parity bit
  tty->c_cflag &= ~CSTOPB; // Clear stop field
  tty->c_cflag &= ~CSIZE; // Clear all bits that set the data size 
  tty->c_cflag |= CS8; // 8 bits per byte
  tty->c_cflag &= ~CRTSCTS; // Disable RTS/CTS hardware flow control
  tty->c_cflag |= CREAD | CLOCAL; // Turn on READ & ignore ctrl lines

  tty->c_lflag &= ~ICANON;
  tty->c_lflag &= ~ECHO; // Disable echo
  tty->c_lflag &= ~ECHOE; // Disable erasure
  tty->c_lflag &= ~ECHONL; // Disable new-line echo
  tty->c_lflag &= ~ISIG; // Disable interpretation of INTR, QUIT and SUSP
  tty->c_iflag &= ~(IXON | IXOFF | IXANY); // Turn off s/w flow ctrl
  tty->c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL); // Disable any special handling of received bytes

  tty->c_oflag &= ~OPOST; // Prevent special interpretation of output bytes (e.g. newline chars)
  tty->c_oflag &= ~ONLCR; // Prevent conversion of newline to carriage return/line feed

  tty->c_cc[VTIME] = 10;    // Wait for up to 1s (10 deciseconds), returning as soon as any data is received.
  tty->c_cc[VMIN] = 0;
}

static bool match(char* s, char* x, int i) {
  return (strlen(x) == i && strncmp(s, x, i) == 0);
}

static std::unique_ptr<char[]> trim_value(char* s, int i) {
  int len = strlen(s);
  int vlen = len - i - 1;
  auto v = std::make_unique<char[]>(vlen+1);
  strncpy(v.get(), &s[i+1], vlen);
  v[vlen] = '\0';
  return v;
}

static bool is_on(char* v) {
  if (strcmp(v, "on") == 0) {
    return true;
  }
  return false;
}

void handle_delay_on(char* v, Delay& d, finyl_track& deck) {
  if (is_on(v)) {
    if (d.on) {
      d.on = false;
      printf("off\n");
    } else {
      double bpm = (deck.meta.bpm/100.0)*deck.get_speed();
      d.setMsize((44100*60)/bpm*1.0);
      d.on = true;
      printf("on\n");
    }
  }
}

void handle_toggle_playing(char* v, finyl_track* t) {
  if (is_on(v)) {
    toggle_playing(*t);
  }
}

void handle_loop_in(char* v, finyl_track* t) {
  if (is_on(v)) {
    loop_in_now(*t);
  }
}

void handle_loop_out(char* v, finyl_track* t) {
  if (is_on(v)) {
    loop_out_now(*t);
  }
}

void handle_vi(char* v, double * vo, double* i, bool flip=false) { //vocal - inst
  char* endptr;
  int n = strtol(v, &endptr, 10);
  if (endptr == v || *endptr != '\0') return;
  if (flip) {
    n = abs(n-4095);
  }
  if (n < 2046) {
    *vo = 1.0;
    *i = n / 2046.0;
  } else {
    *i = 1.0;
    *vo = abs(n-4095.0) / 2046.0;
  }
}

void handle_gain(char* v, double* g, bool flip=false) {
  char* endptr;
  int n = strtol(v, &endptr, 10);
  if (endptr == v || *endptr != '\0') return;
  if (flip) {
    n = abs(n-4095);
  }
  *g = n / 4095.0;
}

void handle_what(char* s) {
  int i = find_char_last(s, ':');
  if (i<1) {
    return;
  }
  
  auto _v = trim_value(s, i);
  char* v = _v.get();

  if (match(s, "pot1", i)) {
    handle_gain(v, &a_gain, true);
  } else if (match(s, "pot0", i)) {
    handle_gain(v, &b_gain, true);
  } else if (match(s, "pot2", i)) {
    handle_vi(v, &a0_gain, &a1_gain);
  } else if (match(s, "pot3", i)) {
    handle_vi(v, &b0_gain, &b1_gain);
  }
  if (adeck != NULL) {
    if (match(s, "button3", i)) {
      handle_toggle_playing(v, adeck);
    }
    else if (match(s, "button4", i)) {
      handle_loop_in(v, adeck);
    }
    else if (match(s, "button10", i)) {
      handle_loop_out(v, adeck);
    } else if (match(s, "button8", i)) {
      handle_delay_on(v, a_delay, *adeck);
    }
  }
  
  if (bdeck != NULL) {
    if (match(s, "button2", i)) {
      handle_toggle_playing(v, bdeck);
    }
    else if (match(s, "button5", i)) { //bdeck
      handle_loop_in(v, bdeck);
    }
    else if(match(s, "button6", i)) {
      handle_loop_out(v, bdeck);
    } else if (match(s, "button9", i)) {
      handle_delay_on(v, b_delay, *bdeck);
    }
  }

  if (adeck != NULL && bdeck!= NULL) {
    if (match(s, "button7", i)) {
      adeck->set_speed(bdeck->get_speed() * ((double)bdeck->meta.bpm / adeck->meta.bpm));
      printf("synced adeck->speed: %lf\n", adeck->get_speed());
    }
    if (match(s, "button1", i)) {
      bdeck->set_speed(adeck->get_speed() * ((double)adeck->meta.bpm / bdeck->meta.bpm));
      printf("synced bdeck->speed: %lf\n", bdeck->get_speed());
    }
  }
}

void* serial(void* args) {
  int serialPort = open("/dev/ttyUSB0", O_RDWR);

  printf("serial\n");
  
  if (serialPort < 0) {
    printf("Error %i from serial open: %s\n", errno, strerror(errno));
    return NULL;
  }

  struct termios tty;
  magic(serialPort, &tty);
  
  if (tcsetattr(serialPort, TCSANOW, &tty) != 0) {
    printf("Error %i from tcsetattr: %s\n", errno, strerror(errno));
    return NULL;
  }
  
  write(serialPort, "init", 4);
  
  char tmp[100];
  int tmplen = 0;
  while(1) {
    char read_buf [256]; //could contain error
    int num_bytes = read(serialPort, &read_buf, sizeof(read_buf));
    if (num_bytes < 0) {
      printf("Error reading: %s\n", strerror(errno));
      return NULL;
    }
    
    int start_i = 0;
    for (int i = 0; i<num_bytes; i++) {
      int linelen = i - start_i; //\n not counted
      char c = read_buf[i];
      if (c != '\n') {
        continue;
      }
      if (tmplen > 0) {
        int len = tmplen + linelen;
        char s[len+1];
        strncpy(s, tmp, tmplen);
        s[tmplen] = '\0';
        strncat(s, &read_buf[start_i], linelen);
        handle_what(s);
        tmplen = 0;
      } else {
        if (linelen == 0) continue;
        //no need to care tmp
        char s[linelen+1];
        strncpy(s, &read_buf[start_i], linelen);
        s[linelen] = '\0';
        handle_what(s);
      }

      start_i = i+1; //i+1 is next to newline
    }

    tmplen = num_bytes-start_i;
    strncpy(tmp, &read_buf[start_i], tmplen);
  }
  
  close(serialPort);
  return NULL;
}

void* _interface(void*) {
  run_interface();
  return NULL;
}

void start_interface() {
  pthread_t interface_thread;
  pthread_create(&interface_thread, NULL, _interface, NULL);
}

void handleKey(char x) {
  if (adeck != NULL) {
    switch (x) {
    case 'z':
      if (a_delay.on) {
        a_delay.on = false;
        printf("delay off\n");
      } else {
        //bpm beats is 44100*60 samples
        //1 beat is 44100*60/bpm samples 
        double bpm = (adeck->meta.bpm/100.0)*adeck->get_speed();
        a_delay.setMsize((44100*60)/bpm*1.0);
        a_delay.on = true;
        printf("delay on: %lf %lf\n", a_delay.drymix, a_delay.feedback);
      }
      break;
    case 'g':
      adeck->playing = !adeck->playing;
      printf("adeck is playing:%d\n", adeck->playing);
      return;
    case 'G':
      adeck->set_speed(1);
      return;
    case 'N':
      a0_gain = 0;
      printf("a0_gain %lf\n", a0_gain);
      return;
    case 'J':
      a0_gain = 1;
      printf("a0_gain %lf\n", a0_gain);
      return;
    case 'n':
      a1_gain = max(a1_gain-0.05, 0.0);
      printf("a1_gain %lf\n", a1_gain);
      return;
    case 'j':
      a1_gain = min(a1_gain+0.05, 1.0);
      printf("a1_gain %lf\n", a1_gain);
      return;
    case 'c':
      if (adeck->cues.size() > 0) {
        adeck->set_index(adeck->cues[0].time * 44.1);
        printf("jumped to %lf\n", adeck->get_refindex());
      }
      return;
    case 'a': {
      adeck->set_speed(adeck->get_speed() + 0.01);
      printf("%lf\n", adeck->get_speed());
      return;
    }
    case 's': {
      adeck->set_speed(adeck->get_speed() - 0.01);
      printf("%lf\n", adeck->get_speed());
      return;
    }
    case 't': {
      set_cue(*adeck);
      printf("adeck->index %lf\n", adeck->get_refindex());
      return;
    }
    case '1':
      /* mark loop in */
      loop_in_now(*adeck);
      return;
    case '!':
      loop_deactivate(*adeck);
      return;
    case '2': {
      loop_out_now(*adeck);
      return;
    }
    case 'v':
      adeck->set_index(adeck->get_refindex() + 300);
      return;
    case 'V':
      adeck->set_index(adeck->get_refindex() - 300);
      return;
    case '5':
      adeck->set_index(adeck->get_refindex() + 3000);
      return;
    case '%':
      adeck->set_index(adeck->get_refindex() - 3000);
      return;

    }
  }
  
  if (bdeck != NULL) {
    switch (x) {
    case 'x':
      if (b_delay.on) {
        b_delay.on = false;
        printf("delay off\n");
      } else {
        //bpm beats is 44100*60 samples
        //1 beat is 44100*60/bpm samples 
        double bpm = (bdeck->meta.bpm/100.0)*bdeck->get_speed();
        b_delay.setMsize((44100*60)/bpm*2);
        b_delay.on = true;
        printf("delay on: %lf %lf\n", b_delay.drymix, b_delay.feedback);
      }
      break;

    case 'h':
      bdeck->playing = !bdeck->playing;
      printf("bdeck is playing:%d\n", bdeck->playing);
      return;
    case 'H':
      bdeck->set_speed(1);
      return;
    case 'M':
      b0_gain = 0;
      printf("b0_gain %lf\n", b0_gain);
      return;
    case 'K':
      b0_gain = 1;
      printf("b0_gain %lf\n", b0_gain);
      return;
    case 'm':
      b1_gain = max(b1_gain-0.05, 0.0);
      printf("b1_gain %lf\n", b1_gain);
      return;
    case 'k':
      b1_gain = min(b1_gain+0.05, 1.0);
      printf("b1_gain %lf\n", b1_gain);
      return;
    case 'C':
      if (bdeck->cues.size() > 0) {
        bdeck->set_index(bdeck->cues[0].time * 44.1);
        printf("jumped to %lf\n", bdeck->get_refindex());
      }
      return;
    case 'A':
      bdeck->set_speed(bdeck->get_speed() + 0.01);
      printf("%lf\n", bdeck->get_speed());
      return;
    case 'S':
      bdeck->set_speed(bdeck->get_speed() - 0.01);
      printf("%lf\n", bdeck->get_speed());
      return;
    case 'y': {
      set_cue(*bdeck);
      printf("bdeck->index %lf\n", bdeck->get_refindex());
      return;
    }
    case '9':
      loop_in_now(*bdeck);
      return;
    case '{':
      loop_deactivate(*bdeck);
      return;
    case '0': {
      loop_out_now(*bdeck);
      return;
    }
    case 'b':
      bdeck->set_index(bdeck->get_refindex() + 300);
      return;
    case 'B':
      bdeck->set_index(bdeck->get_refindex() - 300);
      return;
    case '6':
      bdeck->set_index(bdeck->get_refindex() + 3000);
      return;
    case '&':
      bdeck->set_index(bdeck->get_refindex() - 3000);
      return;

    }
  }
  
  if (adeck != NULL && bdeck != NULL) {
    switch (x) {
    case 'p':
      print_track(*adeck);
      print_track(*bdeck);
      return;
    case 'q': {
      bdeck->set_speed(adeck->get_speed() * ((double)adeck->meta.bpm / bdeck->meta.bpm));
      printf("synced bdeck->speed: %lf\n", bdeck->get_speed());
      return;
    }
    case 'Q': {
      adeck->set_speed(bdeck->get_speed() * ((double)bdeck->meta.bpm / adeck->meta.bpm));
      printf("synced adeck->speed: %lf\n", adeck->get_speed());
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
    load_track_nchannels(&adeck, tid, finyl_a, 2);
    break;
  }
  case 'o': {
    int tid;
    printf("tid:");
    scanf("%d", &tid);
    printf("loading...%d\n", tid);
    load_track_nchannels(&bdeck, tid, finyl_b, 2);
    break;
  }
  case 'I': {
    int tid;
    printf("tid:");
    scanf("%d", &tid);
    printf("loading...%d\n", tid);
    load_track(&adeck, tid, finyl_a);
    break;
  }
  case 'O': {
    int tid;
    printf("tid:");
    scanf("%d", &tid);
    printf("loading...%d\n", tid);
    load_track(&bdeck, tid, finyl_b);
    break;
  }
  case '3':{
    start_interface();
    break;
  }
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

void* key_input(void* args) {
  static struct termios oldt, newt;
  
  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;
  newt.c_lflag &= ~(ICANON);          
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);
  while(finyl_running) {
    handleKey(getchar());                 
  }
  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
  printf("key_input closed\n");
  return NULL;
}

void* controller(void* args) {
  pthread_t k;
  pthread_create(&k, NULL, key_input, NULL);
  pthread_t s;
  pthread_create(&s, NULL, serial, NULL);
  
  pthread_join(k, NULL);
  pthread_cancel(s);
  
  return NULL;
}
