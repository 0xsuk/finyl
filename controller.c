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

void list_playlists() {
  finyl_playlist* pls;
  int size = get_playlists(&pls, usb);

  printf("\n");
  for (int i = 0; i<size; i++) {
    printf("%d %s\n", pls[i].id, pls[i].name);
  }

  free_playlists(pls, size);
}

void list_playlist_tracks() {
  finyl_track_meta* tms;
  int size = get_playlist_tracks(&tms, usb, 2); //2 = ukg

  printf("\n");
  for (int i = 0; i<size; i++) {
    printf("%d %d %s\n", tms[i].id, tms[i].bpm, tms[i].title);
  }

  free_track_metas(tms, size);
}

//load track 5 to adeck
void load_sample_track() {
  finyl_track* t = (finyl_track*)malloc(sizeof(finyl_track));
  finyl_init_track(t);
  get_track(t, usb, 62);
  
  char* files[1] = {t->meta.filepath};
  if (finyl_read_channels_from_files(files, 1, t) == -1) {
    return;
  }
  
  print_track(t);
    
  adeck = t;
}

void* _interface() {
  interface(adeck);

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
    break;
  case 'g':
    adeck->playing = !adeck->playing;
    printf("adeck is playing:%d\n", adeck->playing);
    break;
  case 'G':
    adeck->speed = 1;
    break;
  case 'j':
    a0_gain = max(a0_gain-0.1, 0);
    break;
  case 'k':
    a0_gain = min(a0_gain+0.1, 1);
    break;
  case 'n':
    a1_gain = max(a1_gain-0.1, 0);
    break;
  case 'm':
    a1_gain = min(a1_gain+0.1, 1);
    break;
  case 'c':
    adeck->index = 0;
    break;
  case 'p':
    print_track(adeck);
    break;
  case 'a':
    adeck->speed = adeck->speed + 0.01;
    break;
  case 's':
    adeck->speed = adeck->speed - 0.01;
    break;
  case 't':
    slide_right(adeck);
    break;
  case 'L':
    list_playlists();
    break;
  case 'l':
    list_playlist_tracks();
    break;
  case '0':
    load_sample_track();
    break;
  case 'q': {
    double q = finyl_get_quantized_time(adeck);
    printf("time is %lf\n", q);
    break;
  }
  case 'C':
    adeck->index = 9791.0 * 44.1;
    break;
  case '1':
    /* mark loop in */
    adeck->loop_in = 44.1 * finyl_get_quantized_time(adeck);
    adeck->loop_out = -1.0;
    break;
  case '2': {
    double now = 44.1 * finyl_get_quantized_time(adeck);
    if (adeck->loop_in > now) {
      adeck->loop_in = -1;
    } else {
      adeck->loop_out = now;
    }
    break;
  }

  case '3':
    start_interface();
    break;
  }
}

void* key_input(void* arg) {
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
