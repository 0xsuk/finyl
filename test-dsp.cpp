#include <stdio.h>
#include <string>
#include <memory>
#include "finyl.h"
#include "dsp.h"
#include <alsa/asoundlib.h>
#include <algorithm>
#include "extern.h"
#include <rubberband/RubberBandStretcher.h>

int mindex = 0;
bool running = true;

using rb = RubberBand::RubberBandStretcher;

// void fillBuffer(finyl_buffer& buffer, finyl_stem& stem) {
//   for (int i = 0; i < period_size_2; i=i+2) {
//     if (mindex < stem.msize()) {
//       finyl_sample left, right;
//       stem.get_samples(left, right, mindex);
//       mindex++;
//       if (mindex*3 >= stem.msize()) {
//         buffer[i] = 0;
//         buffer[i+1] = 0;
//         continue;
//       }
//       buffer[i] = left;
//       buffer[i+1] = right;
//     } else {
//       running = false;
//       buffer[i] = 0;
//       buffer[i+1] = 0;
//     }
//   }
// }

double timeratio = 1.2;
rb stretcher(44100, 2, rb::OptionProcessRealTime, timeratio);
int inputSize = 16384;
void rubberband(float** outputPtr, int nframes, finyl_stem& stem) {
  int available;
  while ((available = stretcher.available()) < nframes) {
    int reqd = int(ceil(double(nframes - available) / timeratio));
    reqd = std::max(reqd, int(stretcher.getSamplesRequired()));
    reqd = std::min(reqd, inputSize);
    if (reqd == 0) reqd = 1; //?
    
    float inputLeft[reqd];
    float inputRight[reqd];
    for (int i = 0; i<reqd; i++) {
      if (mindex < stem.msize()) {
        finyl_sample left, right;
        stem.get_samples(left, right, mindex);
        mindex++;
        inputLeft[i] = float(left / 32768.0);
        inputRight[i] = float(right / 32768.0);
      } else {
        inputLeft[i] = 0;
        inputRight[i] = 0;
      }
    }
    
    float* inputs[2] = {inputLeft, inputRight};
    
    stretcher.process(inputs, reqd, false);
  }

  stretcher.retrieve(outputPtr, nframes);
}

int main() {
  //----
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
  //----
  
  float rubleft[period_size];
  float rubright[period_size];
  float* rubout[2] = {rubleft, rubright};
  
  
  while (running) {

    rubberband(rubout, period_size, *stem);
    
    for (int i = 0; i<period_size; i++) {
      int left = int(rubleft[i] * 32768 * 0.5);
      int right = int(rubright[i] * 32768 * 0.5);
      if (abs(left) > 32767 || abs(right) > 32767) {
        printf("bad: %f %f\n", rubleft[i], rubright[i]);
        left = clip_sample(left);
        right = clip_sample(right);
      }
      
      buffer[i*2] = left;
      buffer[i*2+1] = right;
    }
    
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
