#include <sys/resource.h>
#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include "finyl.h"
#include "util.h"
#include "digger.h"
#include "dev.h"
#include "interface.h"
#include <pthread.h>
#include <memory>
#include "action.h"

void slide_right(finyl_track* t) {
  double backup = t->speed;
  double y = 0;
  double x = 0;
  while (y>=0) {
    y  = -x*(x-2);
    t->speed = y;
    x = 0.01 + x;
    usleep(1000);
    printf("t is %lf\n", t->speed);
  }
  t->speed = backup;
}

//n = 0 means load an original file
void load_track_nchannels(finyl_track** dest, int tid, finyl_track_target deck, int n) {
  finyl_track* before = *dest;
  
  finyl_track* t = new finyl_track;
  if (get_track(*t, usb, tid) == -1) {
    printf("failed\n");
    return;
  }
  
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
  
  if (finyl_read_stems_from_files(filepaths, *t) == -1) {
    return;
  }
  
  printf("%s\n", t->meta.filename.data());

  *dest = t;
  if (deck == finyl_a) {
    render_adeck = true;
  } else if (deck == finyl_b) {
    render_bdeck = true;
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

static bool is_y(char* v) {
  if (strcmp(v, "y") == 0) {
    return true;
  }
  return false;
}

void handle_toggle_playing(char* v, finyl_track* t) {
  if (is_y(v)) {
    toggle_playing(t);
  }
}

void handle_loop_in(char* v, finyl_track* t) {
  if (is_y(v)) {
    loop_in_now(t);
  }
}

void handle_loop_out(char* v, finyl_track* t) {
  if (is_y(v)) {
    loop_out_now(t);
  }
}

void handle_gain(char* v, double* g) {
  char* endptr;
  int n = strtol(v, &endptr, 10);
  if (endptr == v || *endptr != '\0') return;
  *g = n / 2046.0;
  printf("gain %lf\n", *g);
}

void handle_what(char* s) {
  int i = find_char_last(s, ':');
  if (i<1) {
    return;
  }
  
  auto _v = trim_value(s, i);
  char* v = _v.get();

  if (match(s, "pot0", i)) {
    printf("pot0\n");
    handle_gain(v, &a0_gain);
  }
  else if (match(s, "pot1", i)) {
    printf("pot1\n");
    handle_gain(v, &a1_gain);
  }
  else if (match(s, "pot2", i)) {
    printf("pot2\n");
    handle_gain(v, &b0_gain);
  }
  else if (match(s, "pot3", i)) {
    printf("pot3\n");
    handle_gain(v, &b1_gain);
  }
  else if (match(s, "button0", i)) {
    printf("button0\n");
    handle_toggle_playing(v, bdeck);
  }
  else if (match(s, "button1", i)) {
    printf("button1\n");
    handle_toggle_playing(v, adeck);
  }
  else if (match(s, "button2", i)) { //bdeck
    printf("button2\n");
    handle_loop_in(v, bdeck);
  }
  else if(match(s, "button3", i)) {
    printf("button3\n");
    handle_loop_out(v, bdeck);
  }
  else if (match(s, "button4", i)) {
    printf("button4\n");
    handle_loop_in(v, adeck);
  }
  else if (match(s, "button5", i)) {
    printf("button5\n");
    handle_loop_out(v, adeck);
  }
  else {
    printf("unknown:");
    printf("%s\n", s);
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
  interface();
  return NULL;
}

void start_interface() {
  pthread_t interface_thread;
  pthread_create(&interface_thread, NULL, _interface, NULL);
}

void handleKey(char x) {
  switch (x) {
  case 'h':
    bdeck->playing = !bdeck->playing;
    printf("bdeck is playing:%d\n", bdeck->playing);
    break;
  case 'g':
    adeck->playing = !adeck->playing;
    printf("adeck is playing:%d\n", adeck->playing);
    break;
  case 'G':
    adeck->speed = 1;
    break;
  case 'H':
    bdeck->speed = 1;
    break;
  case 'N':
    a0_gain = 0;
    printf("a0_gain %lf\n", a0_gain);
    break;
  case 'J':
    a0_gain = 1;
    printf("a0_gain %lf\n", a0_gain);
    break;
  case 'n':
    a1_gain = max(a1_gain-0.05, 0.0);
    printf("a1_gain %lf\n", a1_gain);
    break;
  case 'j':
    a1_gain = min(a1_gain+0.05, 1.5);
    printf("a1_gain %lf\n", a1_gain);
    break;
  case 'M':
    b0_gain = 0;
    printf("b0_gain %lf\n", b0_gain);
    break;
  case 'K':
    b0_gain = 1;
    printf("b0_gain %lf\n", b0_gain);
    break;
  case 'm':
    b1_gain = max(b1_gain-0.05, 0.0);
    printf("b1_gain %lf\n", b1_gain);
    break;
  case 'k':
    b1_gain = min(b1_gain+0.05, 1.5);
    printf("b1_gain %lf\n", b1_gain);
    break;

  case 'c':
    if (adeck->cues.size() > 0) {
      adeck->index = adeck->cues[0].time * 44.1;
      printf("jumped to %lf\n", adeck->index);
    }
    break;
  case 'C':
    if (bdeck->cues.size() > 0) {
      bdeck->index = bdeck->cues[0].time * 44.1;
      printf("jumped to %lf\n", bdeck->index);
    }
    break;
  case 'p':
    print_track(*adeck);
    print_track(*bdeck);
    break;
  case 'a':
    adeck->speed = adeck->speed + 0.01;
    break;
  case 's':
    adeck->speed = adeck->speed - 0.01;
    break;
  case 'A':
    bdeck->speed = bdeck->speed + 0.01;
    break;
  case 'S':
    bdeck->speed = bdeck->speed - 0.01;
    break;
  case 't': {
    adeck->index = finyl_get_quantized_index(*adeck, adeck->index);
    printf("adeck->index %lf\n", adeck->index);
    break;
  }
  case 'y': {
    bdeck->index = finyl_get_quantized_index(*bdeck, bdeck->index);
    printf("bdeck->index %lf\n", bdeck->index);
    break;
  }
  case 'L':
    list_playlists();
    break;
  case 'l': {
    int pid;
    printf("pid:");
    scanf("%d", &pid);
    printf("listing...\n");
    list_playlist_tracks(pid);
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
  // case '9': {
  //   int tid;
  //   printf("tid:");
  //   scanf("%d", &tid);
  //   printf("loading...%d\n", tid);
  //   load_track(&adeck, tid, finyl_a);
  //   break;
  // }
  // case '0': {
  //   int tid;
  //   printf("tid:");
  //   scanf("%d", &tid);
  //   printf("loading...%d\n", tid);
  //   load_track(&bdeck, tid, finyl_b);
  //   break;
  // }
  case 'q': {
    bdeck->speed = adeck->speed * ((double)adeck->meta.bpm / bdeck->meta.bpm);
    printf("synced bdeck->speed: %lf\n", bdeck->speed);
    break;
  }
  case 'Q': {
    adeck->speed = bdeck->speed * ((double)bdeck->meta.bpm / adeck->meta.bpm);
    printf("synced adeck->speed: %lf\n", adeck->speed);
    break;
  }
  case '1':
    /* mark loop in */
    loop_in_now(adeck);
    break;
  case '2': {
    loop_out_now(adeck);
    break;
  }
  case '9':
    loop_in_now(bdeck);
    break;
  case '0': {
    loop_out_now(bdeck);
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
  case 'v':
    adeck->index += 300;
    break;
  case 'b':
    bdeck->index += 300;
    break;
  case 'V':
    adeck->index -= 300;
    break;
  case 'B':
    bdeck->index -= 300;
    break;
  case '5':
    adeck->index += 3000;
    break;
  case '6':
    bdeck->index += 3000;
    break;
  case '%':
    adeck->index -= 3000;
    break;
  case '&':
    bdeck->index -= 3000;
    break;
  case '7': {
    set_wave_range(wave_range*2);
    break;
  }
  case '\'': {
    set_wave_range(wave_range/2);
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
  printf("deck initializing\n");
  
  load_track(&adeck, 1, finyl_a);
  load_track(&bdeck, 1, finyl_b);
  
  printf("deck initialized\n");
  
  pthread_t k;
  pthread_create(&k, NULL, key_input, NULL);
  pthread_t s;
  pthread_create(&s, NULL, serial, NULL);
  
  pthread_join(k, NULL);
  pthread_cancel(s);
  
  return NULL;
}
