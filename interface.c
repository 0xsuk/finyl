#include <SDL2/SDL.h>
#include <math.h>
#include "finyl.h"
#include <X11/Xlib.h>


int amount = 1000000;

int window_width = 1000;
int window_height = 500;

int wave_height = 100;
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

int get_pixel(int ioffset, int range, int window_width) {
  return (ioffset / (float)range) * window_width;
}

float get_scaled_sample(finyl_channel c, int position) {
  float y = finyl_get_sample1(c, position) / 32768.0;
  return y;
}

int get_index(int starti, int x, int range) {
  return starti + (int)((x / (float)window_width) * range);
}

void erase_edge(SDL_Renderer* renderer, int xdiff) {
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
  if (xdiff > 0) {
    SDL_Rect rect = {window_width-xdiff, 0, xdiff, wave_height}; //right edge
    SDL_RenderFillRect(renderer, &rect);
  } else if (xdiff == 0) {
      
  } else {
    SDL_Rect rect = {0, 0, -xdiff, wave_height}; //left edge
    SDL_RenderFillRect(renderer, &rect);
  }
}

void slide(SDL_Renderer* renderer, SDL_Texture* texture, int xdiff) {
  if (xdiff > 0) {
    //slide to left by xdiff pixel
    SDL_Rect src = {xdiff, 0, window_width-xdiff, wave_height};
    SDL_Rect dst = {0, 0, window_width-xdiff, wave_height};

    SDL_RenderCopy(renderer, texture, &src, &dst);
  } else {
    //slide to right by -xdiff pixel
    SDL_Rect src = {0, 0, window_width+xdiff, wave_height};
    SDL_Rect dst = {-xdiff, 0, window_width+xdiff, wave_height};
    SDL_RenderCopy(renderer, texture, &src, &dst);
  }
}

void draw_waveform(SDL_Renderer* renderer, SDL_Texture* texture, finyl_track* t, int starti, int range, int draw_range) {

  SDL_SetRenderDrawColor(renderer, 100, 100, 250, 255); // Waveform color

  int idraw_offset = range-draw_range;
  int idraw_offset_end = range;
  if (draw_range < 0) {
    idraw_offset = 0;
    idraw_offset_end = -draw_range;
  }
  
  for (int i = idraw_offset; i < idraw_offset_end; i=i+2) {
    int x = get_pixel(i, range, window_width);
    int pcmi = i+starti;
    
    float sample = 0;
    if (pcmi>=0) {
      sample = get_scaled_sample(t->channels[0], pcmi);
    }
    
    int y = wave_height_half - sample * wave_height_half;
    SDL_RenderDrawLine(renderer, x, y, x, wave_height_half);
  }
}

int plus_waveform(SDL_Renderer *renderer, SDL_Texture* texture, finyl_track* t, int starti, int range, int wave_y, int previ) {
  SDL_SetRenderTarget(renderer, texture);

  int draw_range = starti - previ;
  int xdiff = (draw_range / (float)range) * window_width;
  int newprevi = get_index(previ, xdiff, range);
  
  slide(renderer, texture, xdiff);
  erase_edge(renderer, xdiff);
  
  draw_waveform(renderer, texture, t, starti, range, draw_range);
  
  SDL_SetRenderTarget(renderer, NULL);
  SDL_Rect dst = {0, wave_y, window_width, wave_height};
  SDL_RenderCopy(renderer, texture, NULL, &dst);
  
  return newprevi;
}

void render_waveform(SDL_Renderer *renderer, SDL_Texture* texture, finyl_track* t, int starti, int range, int wave_y) {
  SDL_SetRenderTarget(renderer, texture);
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
  SDL_RenderClear(renderer);
  
  draw_waveform(renderer, texture, t, starti, range, range);
  
  SDL_SetRenderTarget(renderer, NULL);
  SDL_Rect dst = {0, wave_y, window_width, wave_height};
  SDL_RenderCopy(renderer, texture, NULL, &dst);
}

void render_static_grid(SDL_Renderer* renderer, SDL_Texture* texture, int wave_y) {
  SDL_SetRenderTarget(renderer, texture);
  
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
  SDL_RenderClear(renderer);

  SDL_SetRenderDrawColor(renderer, 255, 0, 250, 255); // Waveform color
  SDL_Rect rect = {window_width / 2 - 1, 0, 2, wave_height};
  SDL_RenderFillRect(renderer, &rect);
  
  SDL_SetRenderTarget(renderer, NULL);
  SDL_Rect dst = {0, wave_y, window_width, wave_height};
  SDL_RenderCopy(renderer, texture, NULL, &dst);
}

finyl_track* tracks_to_free[2];
int tracks_to_free_tail = 0;

void add_track_to_free(finyl_track* t) {
  tracks_to_free[tracks_to_free_tail] = t;
  tracks_to_free_tail++;
}

void free_tracks() {
  for (int i = 0; i<tracks_to_free_tail; i++) {
    finyl_free_track(tracks_to_free[i]);
  }
  tracks_to_free_tail = 0;
}

bool render_adeck = false;
bool render_bdeck = false;

int previ_adeck = 0;
int previ_bdeck = 0;

int interface() {
  get_window_size();
  
  int window_height = wave_height * 2 + 20;
  
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    printf("SDL initialization failed: %s\n", SDL_GetError());
    return 1;
  }

  SDL_Window *window = SDL_CreateWindow("PCM Waveform Display", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, window_width, window_height, SDL_WINDOW_SHOWN);
  if (window == NULL) {
    printf("Window creation failed: %s\n", SDL_GetError());
    return 1;
  }

  SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
  if (renderer == NULL) {
    printf("Renderer creation failed: %s\n", SDL_GetError());
    return 1;
  }

  SDL_Texture* awave_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, window_width, wave_height);
  SDL_Texture* bwave_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, window_width, wave_height);
  SDL_Texture* astatic_grid_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, window_width, wave_height);
  SDL_SetTextureBlendMode(astatic_grid_texture, SDL_BLENDMODE_BLEND);

  SDL_Texture* bstatic_grid_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, window_width, wave_height);
  SDL_SetTextureBlendMode(bstatic_grid_texture, SDL_BLENDMODE_BLEND);
  
  SDL_Event event;
  int fps = 60;
  int desired_delta = 1000 / fps;
  while (finyl_running) {
    int start_msec = SDL_GetTicks();
    
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        goto cleanup;
      }
    }

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);



    free_tracks();
    if (render_adeck) {
      int starti = (int)adeck->index - (int)(amount/2);
      render_waveform(renderer, awave_texture, adeck, starti, amount, 0);
      render_adeck = false;
      previ_adeck = starti;
    } else if (adeck != NULL) {
      int starti = (int)adeck->index - (int)(amount/2);
      previ_adeck = plus_waveform(renderer, awave_texture, adeck, starti, amount, 0, previ_adeck);
    }
    
    if (render_bdeck) {
      int starti = (int)bdeck->index - (int)(amount/2);
      render_waveform(renderer, bwave_texture, bdeck, starti, amount, wave_height+10);
      render_bdeck = false;
      previ_bdeck = starti;
    } else if (bdeck != NULL) {
      int starti = (int)bdeck->index - (int)(amount/2);
      previ_bdeck = plus_waveform(renderer, bwave_texture, bdeck, starti, amount, wave_height+10, previ_bdeck);
    }
    
    render_static_grid(renderer, astatic_grid_texture, 0);
    render_static_grid(renderer, bstatic_grid_texture, wave_height + 10);
        
    SDL_RenderPresent(renderer);
    

    int delta = SDL_GetTicks() - start_msec;
    if (delta < desired_delta) {
      SDL_Delay(desired_delta - delta);
    }
  }

 cleanup:
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}
