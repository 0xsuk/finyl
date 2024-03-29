#include "dsp.h"

void delay(finyl_buffer& buffer, Delay& d) {
  for (int i = 0; i<buffer.size(); i=i+2) {
    auto left = d.buf[d.index];
    auto right = d.buf[d.index+1];
    
    buffer[i] = clip_sample(buffer[i] * d.drymix + left * d.wetmix);
    buffer[i+1] = clip_sample(buffer[i+1] * d.drymix + right * d.wetmix);

    if (d.on) {
      d.buf[d.index] = d.feedback * (left + buffer[i]);
      d.buf[d.index+1] = d.feedback * (right + buffer[i+1]);
    } else {
      d.buf[d.index] = d.feedback * left;
      d.buf[d.index+1] = d.feedback * right;
    }
    
    d.index=d.index+2;
    if (d.index >= d.ssize) {
      d.index = 0;
    }
  }
}
