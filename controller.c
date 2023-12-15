#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include "finyl.h"
#include "digger.h"
#include "dev.h"

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
  finyl_track t;
  get_track(&t, usb, 1);
  print_track(&t);
}

void handleKey(char x) {
  switch (x) {
  case 'h':
    bdeck->playing = !bdeck->playing;
    break;
  case 'g':
    adeck->playing = !adeck->playing;
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
  }
}

void* key_input(void* arg) {
  static struct termios oldt, newt;
  
  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;
  newt.c_lflag &= ~(ICANON);          
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);
  while(1) {
    handleKey(getchar());                 
  }
  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
  return NULL;
}


///TODO list playlists, tracks
///TODO get detailed information of track, and get it as finyl output
