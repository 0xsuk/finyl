#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include "finyl.h"
#include "digger.h"



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
