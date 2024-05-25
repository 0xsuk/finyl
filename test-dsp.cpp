#include <stdio.h>
#include <string>
#include <memory>
#include "thread"
#include "finyl.h"
#include "dev.h"
#include "dsp.h"
#include <alsa/asoundlib.h>
#include <algorithm>
#include "extern.h"
#include "RubberBandStretcher.h"
bool running = true;

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

double timeratio = 1.1;
rb stretcher(44100, 2, rb::OptionProcessRealTime, timeratio);
rb stretcher1(44100, 2, rb::OptionProcessRealTime, timeratio);
int inputSize = 16384;
int rubberband(rb& stretcher, float** outputPtr, int nframes, finyl_stem& stem, int index) {
  int available;
  while ((available = stretcher.available()) < nframes) {
    int reqd = int(ceil(double(nframes - available) / timeratio));
    reqd = std::max(reqd, int(stretcher.getSamplesRequired()));
    reqd = std::min(reqd, inputSize);
    if (reqd == 0) reqd = 1;
    
    float inputLeft[reqd];
    float inputRight[reqd];
    for (int i = 0; i<reqd; i++) {
      if (index < stem.msize()) {
        finyl_sample left, right;
        stem.get_samples(left, right, index);
        index++;
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
  return index;
}

int test_rubberband() {
  //----
  std::unique_ptr<finyl_stem> stem;
  read_stem("beauty.mp3", stem);
  std::unique_ptr<finyl_stem> stem1;
  read_stem("beauty.mp3", stem1);
  printf("size: %d\n", (int)stem->ssize());
  int err = 0;
  snd_pcm_t* handle;
  snd_pcm_uframes_t buffer_size = 1024 * 2;
  period_size = 1024;
  device = "default";
  finyl_setup_alsa(&handle, &buffer_size, &period_size);
  period_size_2 = period_size*2;
  finyl_buffer buffer;
  buffer.resize(period_size_2);
  std::fill(buffer.begin(), buffer.end(), 0);
  //----
  
  float rubleft[period_size];
  float rubright[period_size];
  float* rubout[2] = {rubleft, rubright};
  
  float rubleft1[period_size];
  float rubright1[period_size];
  float* rubout1[2] = {rubleft1, rubright1};
  
  

  int mindex = 1000000;
  
  auto t = std::thread([](){
    char c;
    scanf("%c", &c);
    running = false;
  });
  
  while (running) {

    int newindex;
    
    std::thread t([&]() {
      newindex = rubberband(stretcher, rubout, period_size, *stem, mindex);
    });
    rubberband(stretcher1, rubout1, period_size, *stem1, mindex);
    t.join();
    mindex = newindex;
    
    for (int i = 0; i<period_size; i++) {
      int left = int(rubleft[i] * 32768 * 1.0);
      int right = int(rubright[i] * 32768 * 1.0);
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
  profile();
  t.join();
}



void test_make_stem_buffer(finyl_buffer& buffer, finyl_stem& stem, double& mindex) {

  for (int i = 0; i<period_size_2; i=i+2) {
    stem.get_samples(buffer[i], buffer[i+1], (int)mindex+1);
  }
  mindex = mindex + period_size;
}

int test_fidlib() {
  //----
  int err = 0;
  snd_pcm_t* handle;
  snd_pcm_uframes_t buffer_size = 1024 * 2;
  period_size = 1024;
  device = "default";

  finyl_setup_alsa(&handle, &buffer_size, &period_size);
  period_size_2 = period_size*2;
  finyl_buffer buffer;
  buffer.resize(period_size_2);
  std::fill(buffer.begin(), buffer.end(), 0);
  //----

  
  printf("opened %d\n", (int)period_size_2);
  
  std::unique_ptr<finyl_stem> stem;
  read_stem("outlaw.mp3", stem);

  printf("len %d\n", (int)stem->msize());
  
  // auto bqisoProcessor = BiquadFullKillEQEffect();
  // auto bqisoState = BiquadFullKillEQEffectGroupState();
  
  double mindex = 0;
  
  std::vector<float> input(period_size_2, 0);
  std::vector<float> output(period_size_2, 0);
  
  // bqisoProcessor.process(&bqisoState, input.data(), output.data());
  
  while (running) {

    test_make_stem_buffer(buffer, *stem, mindex);
    printf("mindex: %lf\n", mindex);
    
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
    printf("err: %d\n", err);
  }
  
  printf("alsa closing\n");
  
  err = snd_pcm_drain(handle);
  if (err < 0) {
    printf("snd_pcm_drain failed: %s\n", snd_strerror(err));
  }
  snd_pcm_close(handle);
  printf("alsa closed\n");
}

int main() { test_fidlib(); }
