#include <SDL2/SDL.h>
#include <math.h>
#include "finyl.h"
#include <X11/Xlib.h>


int wave_range = 1000000; //number of samples in range
int wave_iteration_margin = 100;

int window_width = 1000;
int window_height = 500;

int wave_height = 100;
int wave_height_half = 50;

bool render_adeck = false;
bool render_bdeck = false;


void set_wave_range(int _wave_range) {
  if (_wave_range < 10000) {
    return;
  } else if (_wave_range > 20000000){
    return;
  }
  wave_range = _wave_range;
  wave_iteration_margin = wave_range / 10000;
  render_adeck = true;
  render_bdeck = true;
  printf("wave_range %d\n", wave_range);
  printf("wave_iteration_margin %d\n", wave_iteration_margin);
}

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

float get_scaled_sample(finyl_channel& c, int position) {
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

void draw_wave(SDL_Renderer* renderer, finyl_track& t, int x, int i) {
  if (t.channels.size() == 1) {
    float sample = get_scaled_sample(t.channels[0], i);
    int y = wave_height_half - sample*wave_height_half;
    SDL_RenderDrawLine(renderer, x, y, x, wave_height_half);
    return;
  }
  
  int amount0;
  {
    float sample = get_scaled_sample(t.channels[0], i);
    amount0 = sample*wave_height_half;
    int y = wave_height_half - amount0;
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); //white
    SDL_RenderDrawLine(renderer, x, y, x, wave_height_half);
    SDL_SetRenderDrawColor(renderer, 100, 100, 250, 255);
  }
  
  {
    float sample = get_scaled_sample(t.channels[1], i);
    int amount1 = sample*wave_height_half;
    SDL_SetRenderDrawColor(renderer, 100, 100, 250, 255);
    if ((amount0 > 0 && amount1 > 0) || (amount0<0 && amount1<0)) {
      int amount = amount0 + amount1;
      int y = wave_height_half - amount;
      int toy = wave_height_half - amount0;
      SDL_RenderDrawLine(renderer, x, y, x, toy);
    } else {
      int y = wave_height_half - amount1;
      SDL_RenderDrawLine(renderer, x, y, x, wave_height_half);
    }
  }
}

void draw_waveform(SDL_Renderer* renderer, SDL_Texture* texture, finyl_track& t, int starti, int nowi, int range, int draw_range) {
  SDL_SetRenderDrawColor(renderer, 100, 100, 250, 255); // Waveform color

  int idraw_offset = range-draw_range;
  int idraw_offset_end = range;
  if (draw_range < 0) {
    idraw_offset = 0;
    idraw_offset_end = -draw_range;
  }
  
  int prev_pcmi = starti+idraw_offset;
  int beati = finyl_get_quantized_beat_index(t, prev_pcmi);
  if (t.beats[beati].time * 44.1 < prev_pcmi) {
    beati++;
  }
  for (int i = idraw_offset; i < idraw_offset_end; i=i+wave_iteration_margin) {
    int x = get_pixel(i, range, window_width);
    
    int pcmi = i+starti;
    
    bool has_pcm = pcmi>=0 && pcmi < t.length;
    if (!has_pcm) {
      SDL_RenderDrawLine(renderer, x, wave_height_half, x, wave_height_half);
      continue;
    }
    
    draw_wave(renderer, t, x, pcmi);
    
    int beat_pcmi = t.beats[beati].time * 44.1;
    if (prev_pcmi <= beat_pcmi && beat_pcmi < pcmi) {
      SDL_Rect rect = {x-1, 0, 1, wave_height};
      if (t.beats[beati].number == 1) {
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
      } else if (wave_iteration_margin < 200) {
        SDL_SetRenderDrawColor(renderer, 150, 150, 150, 255);
      }
      SDL_RenderFillRect(renderer, &rect);
      SDL_SetRenderDrawColor(renderer, 100, 100, 250, 255);
      beati++;
    }
    prev_pcmi = pcmi;
  }
}

int plus_waveform(SDL_Renderer *renderer, SDL_Texture* texture, finyl_track& t, int nowi, int range, int wave_y, int previ) {
  SDL_SetRenderTarget(renderer, texture);
  int starti = nowi - (int)range/2;
  int draw_range = starti - previ;
  int xdiff = (draw_range / (float)range) * window_width;
  int newprevi = get_index(previ, xdiff, range);
  
  slide(renderer, texture, xdiff);
  erase_edge(renderer, xdiff);
  
  draw_waveform(renderer, texture, t, starti, nowi, range, draw_range);
  
  SDL_SetRenderTarget(renderer, NULL);
  SDL_Rect dst = {0, wave_y, window_width, wave_height};
  SDL_RenderCopy(renderer, texture, NULL, &dst);
  
  return newprevi;
}

void render_waveform(SDL_Renderer *renderer, SDL_Texture* texture, finyl_track& t, int nowi, int range, int wave_y) {
  SDL_SetRenderTarget(renderer, texture);
  int starti = nowi - (int)range/2;

  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
  SDL_RenderClear(renderer);
  
  draw_waveform(renderer, texture, t, starti, nowi, range, range);
  
  SDL_SetRenderTarget(renderer, NULL);
  SDL_Rect dst = {0, wave_y, window_width, wave_height};
  SDL_RenderCopy(renderer, texture, NULL, &dst);
}

void draw_center_line(SDL_Renderer* renderer) {
  SDL_SetRenderDrawColor(renderer, 255, 0, 250, 255); // Waveform color
  SDL_Rect rect = {window_width / 2 - 1, 0, 2, wave_height};
  SDL_RenderFillRect(renderer, &rect);
}

void draw_static_grids(SDL_Renderer* renderer, finyl_track* t, int range) {
  if (t->beats.size() < 2) {
    return;
  }
  
  SDL_SetRenderDrawColor(renderer, 100, 0, 100, 255);

  int dur = t->beats[1].time - t->beats[0].time; //msec
  int samples = dur * 44.1;

  int i = 1;
  while (1) {
    int xl = get_pixel(range/2 - 32*i*samples, range, window_width);
    if (xl>window_width || xl<0) {
      break;
    }
    SDL_Rect rectl = {xl-1, 0, 2, wave_height};
    SDL_RenderFillRect(renderer, &rectl);
    
    int xr = get_pixel(range/2 + 32*i*samples, range, window_width);
    
    SDL_Rect rectr = {xr-1, 0, 2, wave_height};
    SDL_RenderFillRect(renderer, &rectr);
    i++;

  }
}

void render_static_grids(SDL_Renderer* renderer, SDL_Texture* texture, finyl_track* t, int range, int wave_y) {
  SDL_SetRenderTarget(renderer, texture);
  
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
  SDL_RenderClear(renderer);

  draw_center_line(renderer);
  draw_static_grids(renderer, t, range);
  
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
    delete tracks_to_free[i];
  }
  tracks_to_free_tail = 0;
}

int previ_adeck = 0;
int previ_bdeck = 0;

int interface() {
  get_window_size();
  
  /* int window_height = wave_height * 2 + 20; */
  
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
  int fps = 100;
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
      int nowi = (int)adeck->index;
      render_waveform(renderer, awave_texture, *adeck, nowi, wave_range, 0);
      render_adeck = false;
      previ_adeck = nowi - (int)wave_range/2;
    } else if (adeck != NULL) {
      previ_adeck = plus_waveform(renderer, awave_texture, *adeck, (int)adeck->index, wave_range, 0, previ_adeck);
      render_static_grids(renderer, astatic_grid_texture, adeck, wave_range, 0);
    }
    
    if (render_bdeck) {
      int nowi = (int)bdeck->index;
      render_waveform(renderer, bwave_texture, *bdeck, nowi, wave_range, wave_height+10);
      render_bdeck = false;
      previ_bdeck = nowi - (int)wave_range/2;;
    } else if (bdeck != NULL) {
      previ_bdeck = plus_waveform(renderer, bwave_texture, *bdeck, (int)bdeck->index, wave_range, wave_height+10, previ_bdeck);
      render_static_grids(renderer, bstatic_grid_texture, bdeck, wave_range, wave_height + 10);
    }
    
        
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
