#ifndef DSP_H
#define DSP_H

#include "finyl.h"

#define MAX_DELAY_SEC 5
#define MAX_DELAY_SAMPLES 44100*MAX_DELAY_SEC*2

struct Delay {
  finyl_sample buf[MAX_DELAY_SAMPLES] = {0};
  bool on = false;
  int ssize = 0;
  int index = 0;
  double wetmix;
  double drymix;
  double feedback;

  void setMsize(int _msize) {
    int _ssize = _msize*2;
    if (_ssize>MAX_DELAY_SAMPLES) {
      ssize = MAX_DELAY_SAMPLES;
    } else {
      ssize = _ssize;
    }
  }
};
void delay(finyl_buffer& buffer, Delay& d);
#endif
