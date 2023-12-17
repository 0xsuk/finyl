#include <SDL2/SDL.h>
#include <math.h>
#include "finyl.h"
#include <pthread.h>

#define WINDOW_WIDTH 1600
#define WINDOW_HEIGHT 100
#define SAMPLE_RATE 44100

float get_scaled_sample(finyl_channel c, int position) {
  float y = finyl_get_sample1(c, position) / 32768.0;
  return y;
}

void render_waveform_position(SDL_Renderer *renderer, finyl_track* t, int position) {
  int x1 = ((position - 1) / (float)t->length) * WINDOW_WIDTH;
  int y1 = WINDOW_HEIGHT / 2.0 -  get_scaled_sample(t->channels[0], position-1) * (WINDOW_HEIGHT / 2.0);
  
  int x2 = (position / (float)t->length) * WINDOW_WIDTH;
  int y2 = WINDOW_HEIGHT / 2.0 - get_scaled_sample(t->channels[0], position) * (WINDOW_HEIGHT / 2.0);

  SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
}

void write_waveform(SDL_Renderer *renderer, SDL_Texture* texture, finyl_track* t) {
  SDL_SetRenderTarget(renderer, texture);
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
  SDL_RenderClear(renderer);
  
  SDL_SetRenderDrawColor(renderer, 100, 100, 250, 200); // Set draw color to black for the waveform

  for (int i = 1; i < t->length; i=i+2) {
    render_waveform_position(renderer, t, i);
  }

  SDL_SetRenderTarget(renderer, NULL);
}

void write_position(SDL_Renderer* renderer, SDL_Texture* texture, finyl_track* t) {
  SDL_SetRenderTarget(renderer, texture);
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
  SDL_RenderClear(renderer);
  SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
  
  int position = floor(t->index);
  int x = (position / (float)t->length) * WINDOW_WIDTH;
  SDL_Rect rect = {x, 0, 5, WINDOW_HEIGHT};
  
  SDL_RenderFillRect(renderer, &rect);

  SDL_SetRenderTarget(renderer, NULL);
}

void* event_handler() {
  SDL_Event evt;
  while(finyl_running) {
    SDL_WaitEvent(&evt);
    if(evt.type == SDL_QUIT) {
      finyl_running = false;
    }
  }

  return NULL;
}

int interface(finyl_track* t) {
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    printf("SDL initialization failed: %s\n", SDL_GetError());
    return 1;
  }

  SDL_Window *window = SDL_CreateWindow("PCM Waveform Display", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
  if (window == NULL) {
    printf("Window creation failed: %s\n", SDL_GetError());
    return 1;
  }

  SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  if (renderer == NULL) {
    printf("Renderer creation failed: %s\n", SDL_GetError());
    return 1;
  }

  SDL_Texture* waveform_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, WINDOW_WIDTH, WINDOW_HEIGHT);
  SDL_SetTextureBlendMode(waveform_texture, SDL_BLENDMODE_BLEND);
  SDL_Texture* position_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, 0, WINDOW_HEIGHT);
  SDL_SetTextureBlendMode(position_texture, SDL_BLENDMODE_BLEND);
  
  
  write_waveform(renderer, waveform_texture, t);
  SDL_RenderCopy(renderer, waveform_texture, NULL, NULL);
  SDL_RenderPresent(renderer);
  
  pthread_t event_thread;
  pthread_create(&event_thread, NULL, event_handler, NULL);
  
  int fps = 30;
  int duration_millisec = 1000 / fps;
  while (finyl_running) {
    int start_millisec = SDL_GetTicks();
    write_position(renderer, position_texture, t);
    SDL_RenderCopy(renderer, waveform_texture, NULL, NULL);
    SDL_RenderCopy(renderer, position_texture, NULL, NULL);
    SDL_RenderPresent(renderer);
    if (start_millisec < duration_millisec) {
      SDL_Delay(duration_millisec - start_millisec);
    }
  }
  
  
  printf("closing window\n");
  SDL_DestroyTexture(waveform_texture);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  
  return 0;
}
