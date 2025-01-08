#include "spectrum.h"
#include "sdl.h"
#include "interface.h"

std::vector<double> logspace(double start, double stop, int num) {
  std::vector<double> result;
  if (num < 2) {
    printf("Number of points must be at least 2.\n");
    return result;
  }

  double base = 10.0; // Base for log scale (common log)
  double step = (stop - start) / (num - 1);

  for (int i = 0; i < num; i++) {
    result.push_back(pow(base, start + i * step));
  }

  return result;
}

Spectrum::Spectrum(FFTState& _fftState, int _x, int _y, int _w, int _h): num_bars(200), x(_x), y(_y), w(_w), h(_h), fftState(_fftState),
                                         freqs{logspace(std::log10(20), std::log10(20000), num_bars)} {
  magnitude_accs.resize(num_bars);
  dbs.resize(num_bars);
  
  texture = SDL_CreateTexture(gApp.interface->renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, w, h);
  SDL_SetTextureBlendMode(texture.get(), SDL_BLENDMODE_BLEND);
}


void Spectrum::addMagnitude(double freq, float magnitude) {
  for (int i = 0; i<freqs.size(); i++) {
    if (freq<freqs[i]) {
      if (i == 0)  {
        //ignore
      } else {
        magnitude_accs[i-1] += magnitude;
        return;
      }
    }
  }
}

void Spectrum::calcMagnitudeAccs() {
  std::fill(magnitude_accs.begin(), magnitude_accs.end(), 0);
  
  for (int i = 0; i<fftState.out_size; i++) {
    double freq_i = (double)i * gApp.audio->get_sample_rate() / gApp.audio->get_period_size();
    float magnitude_i = sqrt(pow(fftState.left_out[i][0],2) + pow(fftState.left_out[i][1],2));
    
    addMagnitude(freq_i, magnitude_i);
  }
}

void Spectrum::calcDbs() {
  std::fill(dbs.begin(), dbs.end(), 0);

  for (int i = 0; i<magnitude_accs.size(); i++) {
    dbs[i] = std::log10(1.0+magnitude_accs[i]); //adding 1.0 to prevent db becoming negative
  }
}

void Spectrum::drawDbs() {
  double max_db = 2.8;
  
  int rect_w = w / num_bars;
    
  for (int i = 0; i<num_bars; i++) {
    double db_percent = dbs[i]/max_db;
    db_percent = std::min(db_percent, 1.0);
    int rect_y = h*(1-db_percent);
    
    int rect_x = i * rect_w;

    SDL_Rect rect = {rect_x, rect_y, rect_w, static_cast<int>(h * db_percent)};
    SDL_RenderFillRect(gApp.interface->renderer, &rect);
  }

}

void Spectrum::draw() {
  SDL_SetRenderTarget(gApp.interface->renderer, texture.get());
  SDL_SetRenderDrawColor(gApp.interface->renderer, 0, 0, 0, 0);
  SDL_RenderClear(gApp.interface->renderer);
  
  SDL_SetRenderDrawColor(gApp.interface->renderer, 255, 0, 250, 200);
  
  calcMagnitudeAccs();
  calcDbs();
  drawDbs();
  
  SDL_SetRenderTarget(gApp.interface->renderer, NULL);
  SDL_Rect dst = {x, y, w, h};
  SDL_RenderCopy(gApp.interface->renderer, texture.get(), NULL, &dst);
}
