#include <SDL2/SDL.h>
#include <math.h>
#include "finyl.h"
#include <X11/Xlib.h>


int amount = 1000000;

int window_width = 1000;
int window_height = 500;

int wave_height_half = 50;

int get_window_size() {
  Display* disp = XOpenDisplay(NULL);
  if (disp == NULL) {
    fprintf(stderr, "Unable to connect to display\n");
    return 1;
  }

  int screenNum = DefaultScreen(disp);
  window_width = DisplayWidth(disp, screenNum);
  window_height = DisplayHeight(disp, screenNum);

  printf("Screen Width: %d\n", window_width);
  printf("Screen Height: %d\n", window_height);

  XCloseDisplay(disp);
  return 0;
}

float get_scaled_sample(finyl_channel c, int position) {
  float y = finyl_get_sample1(c, position) / 32768.0;
  return y;
}


//draws from start `amount' much
void render_waveform(SDL_Renderer *renderer, finyl_track* t, int pcm_start_index, int amount, int wave_y) {
  SDL_SetRenderDrawColor(renderer, 100, 100, 250, 255); // Waveform color

  for (int i = 0; i < amount; i=i+20) {
    int x = (i / (float)amount) * window_width;
    
    int pcmi = i+pcm_start_index;
    
    float sample = 0;
    if (pcmi>=0) {
      sample = get_scaled_sample(t->channels[0], pcmi);
    }
    
    int y = wave_y + wave_height_half - sample * wave_height_half;
    SDL_RenderDrawLine(renderer, x, y, x, wave_y+wave_height_half);
  }

  
  SDL_SetRenderDrawColor(renderer, 255, 0, 250, 255); // Waveform color
  SDL_Rect rect = {window_width / 2 - 2, wave_y, 4, wave_height_half*2};
  SDL_RenderFillRect(renderer, &rect);
}

int interface() {
  get_window_size();
  
  int window_height = wave_height_half * 4 + 20;
  
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    printf("SDL initialization failed: %s\n", SDL_GetError());
    return 1;
  }

  SDL_Window *window = SDL_CreateWindow("PCM Waveform Display", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, window_width, window_height, SDL_WINDOW_SHOWN);
  if (window == NULL) {
    printf("Window creation failed: %s\n", SDL_GetError());
    return 1;
  }

  SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  if (renderer == NULL) {
    printf("Renderer creation failed: %s\n", SDL_GetError());
    return 1;
  }

  int scroll_position = 0;
  int scroll_speed = 1; // Adjust this to change the scrolling speed

  SDL_Event event;
  while (finyl_running) {
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        finyl_running = false;
      }
    }

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Background color
    SDL_RenderClear(renderer);


    render_waveform(renderer, adeck, (int)adeck->index - (int)(amount/2), amount, 0);
    render_waveform(renderer, bdeck, (int)bdeck->index - (int)(amount/2), amount, wave_height_half*2 + 10);
    
    SDL_RenderPresent(renderer);


    SDL_Delay(16); // Approximately 60 frames per second
  }

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}
