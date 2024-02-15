#include <stdio.h>
#include <string>
#include <memory>
#include "finyl.h"
#include "dsp.h"
#include <alsa/asoundlib.h>
#include <algorithm>
#include "extern.h"

int mindex = 0;
bool running = true;

void fillBuffer(finyl_buffer& buffer, finyl_stem& stem) {
  for (int i = 0; i < period_size_2; i=i+2) {
    if (mindex < stem.msize()) {
      finyl_sample left, right;
      stem.get_samples(left, right, mindex);
      mindex++;
      if (mindex*3 >= stem.msize()) {
        buffer[i] = 0;
        buffer[i+1] = 0;
        continue;
      }
      buffer[i] = left;
      buffer[i+1] = right;
    } else {
      running = false;
      buffer[i] = 0;
      buffer[i+1] = 0;
    }
  }
}

int main() {
  Delay d;
  d.setMsize(27852*2); //sample is 95 beats. 44100*60/95 = 27852 samples for 1beat
  
  std::unique_ptr<finyl_stem> stem;
  read_stem("beauty.mp3", stem);

  printf("size: %d\n", (int)stem->ssize());

  int err = 0;
  snd_pcm_t* handle;
  snd_pcm_uframes_t buffer_size = 1024 * 2;
  period_size = 1024;
  
  device = "default";

  finyl_setup_alsa(&handle, &buffer_size, &period_size);
  period_size_2 = period_size*2;
  printf("buffer_size %ld, period_size %ld\n", buffer_size, period_size);
  finyl_buffer buffer;
  buffer.resize(period_size_2);
  std::fill(buffer.begin(), buffer.end(), 0);
  
  while (running) {

    fillBuffer(buffer, *stem);
    delay(buffer, d);
    
    
    err = snd_pcm_writei(handle, buffer.data(), period_size);
    if (err == -EPIPE) {
      printf("Underrun occurred: %s\n", snd_strerror(err));
      snd_pcm_prepare(handle);
    } else if (err == -EAGAIN) {
      printf("eagain\n");
    } else if (err < 0) {
      printf("error %s\n", snd_strerror(err));
      break;
    }
  }
  
  err = snd_pcm_drain(handle);
  if (err < 0) {
    printf("snd_pcm_drain failed: %s\n", snd_strerror(err));
  }
  snd_pcm_close(handle);
  printf("alsa closed\n");
}
