#include "spectrum.h"
#include "sdl.h"
#include "interface.h"

Spectrum::Spectrum(FFTState& _fftState): x(200), y(200), w(400), h(400), fftState(_fftState) {
  texture = SDL_CreateTexture(gApp.interface->renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, w, h);
  SDL_SetTextureBlendMode(texture.get(), SDL_BLENDMODE_BLEND);
}

void Spectrum::draw() {
  SDL_SetRenderTarget(gApp.interface->renderer, texture.get());
  SDL_SetRenderDrawColor(gApp.interface->renderer, 0, 0, 0, 0);
  SDL_RenderClear(gApp.interface->renderer);
  
  SDL_SetRenderDrawColor(gApp.interface->renderer, 255, 0, 250, 200);
  
  int rect_w = w / fftState.out_size;
  for (int i = 0; i<fftState.out_size; i++) {
    float mag = sqrt(pow(fftState.left_out[i][0],2) + pow(fftState.left_out[i][1],2)) / 100;
    float magnitude = std::fmin(mag*h, h);
    int rect_y = h - magnitude;
    
    int rect_x = i * rect_w;
    
    SDL_Rect rect = {rect_x, rect_y, rect_w, (int)magnitude};
    SDL_RenderFillRect(gApp.interface->renderer, &rect);
  }
  
  //read fftState
  // fftState.left_out;


  SDL_SetRenderTarget(gApp.interface->renderer, NULL);
  SDL_Rect dst = {x, y, w, h};
  SDL_RenderCopy(gApp.interface->renderer, texture.get(), NULL, &dst);
}
