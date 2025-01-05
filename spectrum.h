#ifndef SPECTRUM_H
#define SPECTRUM_H

#include "dsp.h"
#include "sdl.h"

class Spectrum {
 public:
  Spectrum(FFTState& _fftState);
  void draw();

 private:
  int x;
  int y;
  int w;
  int h;

  Texture texture;
  
  FFTState& fftState;
};

#endif
