#include <sys/resource.h>
#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include "finyl.h"
#include "digger.h"
#include "dev.h"
#include "interface.h"
#include <pthread.h>

double max(double a, double b) {
  return a>b ? a : b;
}

double min(double a, double b) {
  return a<b ? a : b;
}

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

//load track 5 to adeck
void load_track(finyl_track** dest, int tid, finyl_track_target deck) {
  finyl_track* before = *dest;
  
  finyl_track* t = (finyl_track*)malloc(sizeof(finyl_track));
  if (get_track(t, usb, tid) == -1) {
    printf("failed\n");
    return;
  }
  
  char* files[1] = {t->meta.filepath};
  if (finyl_read_channels_from_files(files, 1, t) == -1) {
    return;
  }
  
  print_track(t);
  
  *dest = t;
  if (deck == finyl_a) {
    render_adeck = true;
  } else if (deck == finyl_b) {
    render_bdeck = true;
  }

  if (before != NULL) {
    add_track_to_free(before);
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    printf("Memory usage: %ld kilobytes\n", usage.ru_maxrss);
  }
}

void load_track_2channels(finyl_track** dest, int tid, finyl_track_target deck) {
  finyl_track* before = *dest;
  
  finyl_track* t = (finyl_track*)malloc(sizeof(finyl_track));
  if (get_track(t, usb, tid) == -1) {
    printf("failed\n");
    return;
  }
  
  
  if (t->meta.channels_size < 2) {
    finyl_free_track(t);
    printf("not enough channels\n");
    return;
  }
  
  if (finyl_read_channels_from_files(t->meta.channel_filepaths, 2, t) == -1) {
    return;
  }
  
  print_track(t);
  
  *dest = t;
  if (deck == finyl_a) {
    render_adeck = true;
  } else if (deck == finyl_b) {
    render_bdeck = true;
  }

  if (before != NULL) {
    add_track_to_free(before);
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    printf("Memory usage: %ld kilobytes\n", usage.ru_maxrss);
  }
  
}

void* _interface() {
  interface(adeck, bdeck);

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
  case 'j':
    a0_gain = max(a0_gain-0.05, 0);
    printf("a0_gain %lf\n", a0_gain);
    break;
  case 'J':
    a1_gain = max(a1_gain-0.05, 0);
    printf("a1_gain %lf\n", a1_gain);
    break;
  case 'k':
    a0_gain = min(a0_gain+0.05, 2);
    printf("a0_gain %lf\n", a0_gain);
    break;
  case 'K':
    a1_gain = min(a1_gain+0.05, 2);
    printf("a1_gain %lf\n", a1_gain);
    break;
  case 'n':
    b0_gain = max(b0_gain-0.05, 0);
    printf("b0_gain %lf\n", b0_gain);
    break;
  case 'N':
    b1_gain = max(b1_gain-0.05, 0);
    printf("b1_gain %lf\n", b1_gain);
    break;
  case 'm':
    b0_gain = min(b0_gain+0.1, 2);
    printf("b0_gain %lf\n", b0_gain);
    break;
  case 'M':
    b1_gain = min(b1_gain+0.05, 2);
    printf("b1_gain %lf\n", b1_gain);
    break;

  case 'c':
    if (adeck->cues_size > 0) {
      adeck->index = adeck->cues[0].time * 44.1;
      printf("jumped to %lf\n", adeck->index);
    }
    break;
  case 'C':
    if (bdeck->cues_size > 0) {
      bdeck->index = bdeck->cues[0].time * 44.1;
      printf("jumped to %lf\n", bdeck->index);
    }
    break;
  case 'p':
    print_track(adeck);
    print_track(bdeck);
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
    double millisec = finyl_get_quantized_time(adeck);
    adeck->index = millisec * 44.1;
    printf("adeck->index %lf\n", adeck->index);
    break;
  }
  case 'T': {
    double millisec = finyl_get_quantized_time(bdeck);
    bdeck->index = millisec * 44.1;
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
    load_track_2channels(&adeck, tid, finyl_a);
    break;
  }
  case 'o': {
    int tid;
    printf("tid:");
    scanf("%d", &tid);
    printf("loading...%d\n", tid);
    load_track_2channels(&bdeck, tid, finyl_b);
    break;
  }
  case '9': {
    int tid;
    printf("tid:");
    scanf("%d", &tid);
    printf("loading...%d\n", tid);
    load_track(&adeck, tid, finyl_a);
    break;
  }
  case '0': {
    int tid;
    printf("tid:");
    scanf("%d", &tid);
    printf("loading...%d\n", tid);
    load_track(&bdeck, tid, finyl_b);
    break;
  }
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
    adeck->loop_in = 44.1 * finyl_get_quantized_time(adeck);
    adeck->loop_out = -1.0;
    printf("adeck loop in: %lf\n", adeck->loop_in);
    break;
  case '2': {
    double now = 44.1 * finyl_get_quantized_time(adeck);
    if (adeck->loop_in > now) {
      adeck->loop_in = -1;
    } else {
      adeck->loop_out = now;
      printf("adeck loop out: %lf\n", adeck->loop_out);
    }
    break;
  }
  case '!':
    /* mark loop in */
    bdeck->loop_in = 44.1 * finyl_get_quantized_time(bdeck);
    bdeck->loop_out = -1.0;
    printf("bdeck loop in: %lf\n", bdeck->loop_in);
    break;
  case '`': {
    double now = 44.1 * finyl_get_quantized_time(bdeck);
    if (bdeck->loop_in > now) {
      bdeck->loop_in = -1;
    } else {
      bdeck->loop_out = now;
      printf("bdeck loop out: %lf\n", bdeck->loop_out);
    }
    break;
  }
  case '3':{
    start_interface();
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

void* key_input(void* arg) {
  printf("deck initializing\n");
  
  load_track(&adeck, 1, finyl_a);
  load_track(&bdeck, 1, finyl_b);
  
  printf("deck initialized\n");
  
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
  fflush(stdout);
  return NULL;
}


///TODO list playlists, tracks
///TODO get detailed information of track, and get it as finyl output
