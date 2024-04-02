#include <SDL2/SDL.h>
#include <math.h>
#include "finyl.h"
#include "interface.h"
#include <X11/Xlib.h>
#include "extern.h"

Interface interface;

void set_wave_range(Interface& itf, int wave_range) {
  if (wave_range < 130000) {
    return;
  } else if (wave_range > 4000000){
    return;
  }
  itf.wave_range = wave_range;
  itf.wave_iteration_margin = wave_range / 10000;
  itf.render_adeck = true;
  itf.render_bdeck = true;
  printf("wave_range %d\n", wave_range);
  printf("wave_iteration_margin %d\n", itf.wave_iteration_margin);
}

int get_window_size(int& width, int& height) {
  Display* disp = XOpenDisplay(NULL);
  if (disp == NULL) {
    fprintf(stderr, "Unable to connect to display\n");
    return -1;
  }

  int screenNum = DefaultScreen(disp);
  width = DisplayWidth(disp, screenNum)/2;
  height = DisplayHeight(disp, screenNum)/2;

  XCloseDisplay(disp);
  return 0;
}

int get_pixel(int ioffset, int range, int window_width) {
  return (ioffset / (float)range) * window_width;
}

float get_scaled_left_sample(finyl_stem& s, int mindex) {
  auto left = s[mindex*2];
  return left / 32768.0;
}

int get_index(int win_width, int starti, int x, int range) {
  return starti + (int)((x / (float)win_width) * range);
}

void erase_edge(Interface& itf, int xdiff) {
  SDL_SetRenderDrawColor(itf.renderer, 0, 0, 0, 255);
  if (xdiff > 0) {
    SDL_Rect rect = {itf.win_width-xdiff, 0, xdiff, itf.wave_height}; //right edge
    SDL_RenderFillRect(itf.renderer, &rect);
  } else if (xdiff == 0) {
      
  } else {
    SDL_Rect rect = {0, 0, -xdiff, itf.wave_height}; //left edge
    SDL_RenderFillRect(itf.renderer, &rect);
  }
}

void slide(Interface& itf, SDL_Texture* texture, int xdiff) {
  if (xdiff > 0) {
    //slide to left by xdiff pixel
    SDL_Rect src = {xdiff, 0, itf.win_width-xdiff, itf.wave_height};
    SDL_Rect dst = {0, 0, itf.win_width-xdiff, itf.wave_height};

    SDL_RenderCopy(itf.renderer, texture, &src, &dst);
  } else {
    //slide to right by -xdiff pixel
    SDL_Rect src = {0, 0, itf.win_width+xdiff, itf.wave_height};
    SDL_Rect dst = {-xdiff, 0, itf.win_width+xdiff, itf.wave_height};
    SDL_RenderCopy(itf.renderer, texture, &src, &dst);
  }
}

void draw_wave(Interface& itf, finyl_track& t, int x, int mindex) {
  if (t.stems_size == 1) {
    float sample = get_scaled_left_sample(*t.stems[0], mindex);
    int y = itf.wave_height_half - sample*itf.wave_height_half;
    SDL_RenderDrawLine(itf.renderer, x, y, x, itf.wave_height_half);
    return;
  }
  
  int amount0;
  {
    float sample = get_scaled_left_sample(*t.stems[0], mindex);
    amount0 = sample*itf.wave_height_half;
    int y = itf.wave_height_half - amount0;
    SDL_SetRenderDrawColor(itf.renderer, 255, 255, 255, 255); //white
    SDL_RenderDrawLine(itf.renderer, x, y, x, itf.wave_height_half);
    SDL_SetRenderDrawColor(itf.renderer, 100, 100, 250, 255);
  }
  
  {
    float sample = get_scaled_left_sample(*t.stems[1], mindex);
    int amount1 = sample*itf.wave_height_half;
    SDL_SetRenderDrawColor(itf.renderer, 100, 100, 250, 255);
    if ((amount0 > 0 && amount1 > 0) || (amount0<0 && amount1<0)) {
      int amount = amount0 + amount1;
      int y = itf.wave_height_half - amount;
      int toy = itf.wave_height_half - amount0;
      SDL_RenderDrawLine(itf.renderer, x, y, x, toy);
    } else {
      int y = itf.wave_height_half - amount1;
      SDL_RenderDrawLine(itf.renderer, x, y, x, itf.wave_height_half);
    }
  }
}

void draw_waveform(Interface& itf, SDL_Texture* texture, finyl_track& t, int starti, int nowi, int draw_range) {
  SDL_SetRenderDrawColor(itf.renderer, 100, 100, 250, 255); // Waveform color

  int idraw_offset = itf.wave_range-draw_range;
  int idraw_offset_end = itf.wave_range;
  if (draw_range < 0) {
    idraw_offset = 0;
    idraw_offset_end = -draw_range;
  }
  
  int prev_pcmi = starti+idraw_offset;
  
  int beati = finyl_get_quantized_beat_index(t, prev_pcmi);
  if (t.beats[beati].time * 44.1 < prev_pcmi) {
    beati++;
  }

  for (int i = idraw_offset; i < idraw_offset_end; i=i+itf.wave_iteration_margin) {
    int x = get_pixel(i, itf.wave_range, itf.win_width);
    int pcmi = i+starti;
    
    //horizontal line
    bool has_pcm = pcmi>=0 && pcmi < t.get_refmsize();
    if (!has_pcm) {
      SDL_RenderDrawLine(itf.renderer, x, itf.wave_height_half, x, itf.wave_height_half);
      continue;
    }
    
    //beat grid
    int beat_pcmi = t.beats[beati].time * 44.1;
    if (prev_pcmi <= beat_pcmi && beat_pcmi < pcmi) {
      SDL_Rect rect = {x, 0, 0, itf.wave_height};
      if (t.beats[beati].number == 1) {
        SDL_SetRenderDrawColor(itf.renderer, 255, 255, 255, 255);
        SDL_RenderFillRect(itf.renderer, &rect);
        SDL_SetRenderDrawColor(itf.renderer, 100, 100, 250, 255);
      } else if (itf.wave_iteration_margin < 200) {
        SDL_SetRenderDrawColor(itf.renderer, 150, 150, 150, 255);
        SDL_RenderFillRect(itf.renderer, &rect);
        SDL_SetRenderDrawColor(itf.renderer, 100, 100, 250, 255);
      }
      beati++;
    }
    
    //cue
    for (auto cue = t.cues.begin(); cue!=t.cues.end(); cue++) {
      auto cuei = cue->time*44.1;
      if (prev_pcmi < cuei  && cuei < pcmi) {
        SDL_SetRenderDrawColor(itf.renderer, 255, 71, 0, 255);
        SDL_RenderDrawLine(itf.renderer, x, 0, x, 10);
        SDL_SetRenderDrawColor(itf.renderer, 100, 100, 250, 255);
      }
    }
    
    draw_wave(itf, t, x, pcmi);
    
    prev_pcmi = pcmi;
  }
}

int plus_waveform(Interface& itf, SDL_Texture* texture, finyl_track& t, int nowi, int wave_y, int previ) {
  SDL_SetRenderTarget(itf.renderer, texture);
  int starti = nowi - (int)itf.wave_range/2;
  int draw_range = starti - previ;
  int xdiff = (draw_range / (float)itf.wave_range) * itf.win_width;
  int newprevi = get_index(itf.win_width, previ, xdiff, itf.wave_range);
  
  slide(itf, texture, xdiff);
  erase_edge(itf, xdiff);
  
  draw_waveform(itf, texture, t, starti, nowi, draw_range);
  
  SDL_SetRenderTarget(itf.renderer, NULL);
  SDL_Rect dst = {0, wave_y, itf.win_width, itf.wave_height};
  SDL_RenderCopy(itf.renderer, texture, NULL, &dst);
  
  return newprevi;
}

void render_waveform(Interface& itf, SDL_Texture* texture, finyl_track& t, int nowi, int wave_y) {
  SDL_SetRenderTarget(itf.renderer, texture);
  int starti = nowi - (int)itf.wave_range/2;

  SDL_SetRenderDrawColor(itf.renderer, 0, 0, 0, 0);
  SDL_RenderClear(itf.renderer);
  
  draw_waveform(itf, texture, t, starti, nowi, itf.wave_range);
  
  SDL_SetRenderTarget(itf.renderer, NULL);
  SDL_Rect dst = {0, wave_y, itf.win_width, itf.wave_height};
  SDL_RenderCopy(itf.renderer, texture, NULL, &dst);
}

void draw_center_line(Interface& itf) {
  SDL_SetRenderDrawColor(itf.renderer, 255, 0, 250, 255); // Waveform color
  SDL_Rect rect = {itf.win_width / 2 - 1, 0, 2, itf.wave_height};
  SDL_RenderFillRect(itf.renderer, &rect);
}

void draw_static_grids(Interface& itf, finyl_track* t) {
  if (t->beats.size() < 2) {
    return;
  }
  
  SDL_SetRenderDrawColor(itf.renderer, 100, 0, 100, 255);

  int dur = t->beats[1].time - t->beats[0].time; //msec
  int samples = dur * 44.1;

  int i = 1;
  while (1) {
    int xl = get_pixel(itf.wave_range/2 - 32*i*samples, itf.wave_range, itf.win_width);
    if (xl>itf.win_width || xl<0) {
      break;
    }
    SDL_Rect rectl = {xl-1, 0, 2, itf.wave_height};
    SDL_RenderFillRect(itf.renderer, &rectl);
    
    int xr = get_pixel(itf.wave_range/2 + 32*i*samples, itf.wave_range, itf.win_width);
    
    SDL_Rect rectr = {xr-1, 0, 2, itf.wave_height};
    SDL_RenderFillRect(itf.renderer, &rectr);
    i++;

  }
}

void render_static_grids(Interface& itf, SDL_Texture* texture, finyl_track* t, int wave_y) {
  SDL_SetRenderTarget(itf.renderer, texture);
  
  SDL_SetRenderDrawColor(itf.renderer, 0, 0, 0, 0);
  SDL_RenderClear(itf.renderer);

  draw_center_line(itf);
  draw_static_grids(itf, t);
  
  SDL_SetRenderTarget(itf.renderer, NULL);
  SDL_Rect dst = {0, wave_y, itf.win_width, itf.wave_height};
  SDL_RenderCopy(itf.renderer, texture, NULL, &dst);
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

int run_interface() {
  if (get_window_size(interface.win_width, interface.win_height) == -1) {
    return 1;
  }
  
  interface.win_width = 900;
  interface.win_height = 600;
  
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    printf("SDL initialization failed: %s\n", SDL_GetError());
    return 1;
  }

  SDL_Window *window = SDL_CreateWindow("PCM Waveform Display", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, interface.win_width, interface.win_height, SDL_WINDOW_SHOWN);
  if (window == NULL) {
    printf("Window creation failed: %s\n", SDL_GetError());
    return 1;
  }

  interface.renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
  if (interface.renderer == NULL) {
    printf("Renderer creation failed: %s\n", SDL_GetError());
    return 1;
  }

  interface.tx_awave = SDL_CreateTexture(interface.renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, interface.win_width, interface.wave_height);
  interface.tx_bwave = SDL_CreateTexture(interface.renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, interface.win_width, interface.wave_height);
  interface.tx_asg = SDL_CreateTexture(interface.renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, interface.win_width, interface.wave_height);
  SDL_SetTextureBlendMode(interface.tx_asg, SDL_BLENDMODE_BLEND);
  interface.tx_bsg = SDL_CreateTexture(interface.renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, interface.win_width, interface.wave_height);
  SDL_SetTextureBlendMode(interface.tx_bsg, SDL_BLENDMODE_BLEND);
  
  SDL_Event event;
  int desired_delta = 1000 / fps;
  while (finyl_running) {
    int start_msec = SDL_GetTicks();
    
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        goto cleanup;
      }
    }

    SDL_SetRenderDrawColor(interface.renderer, 0, 0, 0, 255);
    SDL_RenderClear(interface.renderer);


    free_tracks();
    if (interface.render_adeck) {
      int nowi = (int)adeck->get_refindex();
      render_waveform(interface, interface.tx_awave, *adeck, nowi, 0);
      interface.render_adeck = false;
      previ_adeck = nowi - (int)interface.wave_range/2;
    } else if (adeck != NULL) {
      previ_adeck = plus_waveform(interface, interface.tx_awave, *adeck, (int)adeck->get_refindex(), 0, previ_adeck);
      render_static_grids(interface, interface.tx_asg, adeck, 0);
    }
    
    if (interface.render_bdeck) {
      int nowi = (int)bdeck->get_refindex();
      render_waveform(interface, interface.tx_bwave, *bdeck, nowi, interface.wave_height+10);
      interface.render_bdeck = false;
      previ_bdeck = nowi - (int)interface.wave_range/2;;
    } else if (bdeck != NULL) {
      previ_bdeck = plus_waveform(interface, interface.tx_bwave, *bdeck, (int)bdeck->get_refindex(), interface.wave_height+10, previ_bdeck);
      render_static_grids(interface, interface.tx_bsg, bdeck, interface.wave_height + 10);
    }
    
        
    SDL_RenderPresent(interface.renderer);
    

    int delta = SDL_GetTicks() - start_msec;
    if (delta < desired_delta) {
      SDL_Delay(desired_delta - delta);
    }
  }

 cleanup:
  SDL_DestroyRenderer(interface.renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}
