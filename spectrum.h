#ifndef SPECTRUM_H
#define SPECTRUM_H

#include "dsp.h"
#include "sdl.h"

class Spectrum {
 public:
  Spectrum(FFTState& _fftState, int _x, int _y, int _w, int _h);
  void draw();

 private:
  void addMagnitude(double freq, float magnitude);
  void calcMagnitudeAccs();
  void calcDbs();
  void drawDbs();
  
  int num_bars;
  int x;
  int y;
  int w;
  int h;

  Texture texture;
  
  FFTState& fftState;


  //log spaced freqs
  const std::vector<double> freqs;
  std::vector<float> magnitude_accs;
  std::vector<double> dbs;

};

#endif
