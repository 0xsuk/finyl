#include "waveform.h"
#include "interface.h"

WaveForm::WaveForm(Interface& _itf):
  render_adeck(false),
  render_bdeck(false),
  wave_range(1000000),
  wave_height(100),
  wave_height_half(50),
  wave_iteration_margin(100),
  itf(_itf){
  tx_awave = SDL_CreateTexture(itf.renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, itf.win_width, wave_height);
  tx_bwave = SDL_CreateTexture(itf.renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, itf.win_width, wave_height);
  tx_asg = SDL_CreateTexture(itf.renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, itf.win_width, wave_height);
  SDL_SetTextureBlendMode(tx_asg, SDL_BLENDMODE_BLEND);
  tx_bsg = SDL_CreateTexture(itf.renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, itf.win_width, wave_height);
  SDL_SetTextureBlendMode(tx_bsg, SDL_BLENDMODE_BLEND);

};

void WaveForm::set_range(int _range) {
  if (_range < 130000) {
    return;
  } else if (_range > 4000000){
    return;
  }
  wave_range = _range;
  wave_iteration_margin = _range / 10000;
  render_adeck = true;
  render_bdeck = true;
  printf("wave_range %d\n", _range);
  printf("wave_iteration_margin %d\n", wave_iteration_margin);
}

void WaveForm::double_range() {
  set_range(wave_range*2);
}

void WaveForm::half_range() {
  set_range(wave_range/2);
}
void WaveForm::erase_edge(int xdiff) {
  SDL_SetRenderDrawColor(itf.renderer, 0, 0, 0, 255);
  if (xdiff > 0) {
    SDL_Rect rect = {itf.win_width-xdiff, 0, xdiff, wave_height}; //right edge
    SDL_RenderFillRect(itf.renderer, &rect);
  } else if (xdiff == 0) {
      
  } else {
    SDL_Rect rect = {0, 0, -xdiff, wave_height}; //left edge
    SDL_RenderFillRect(itf.renderer, &rect);
  }
}

void WaveForm::slide(SDL_Texture* texture, int xdiff) {
  if (xdiff > 0) {
    //slide to left by xdiff pixel
    SDL_Rect src = {xdiff, 0, itf.win_width-xdiff, wave_height};
    SDL_Rect dst = {0, 0, itf.win_width-xdiff, wave_height};

    SDL_RenderCopy(itf.renderer, texture, &src, &dst);
  } else {
    //slide to right by -xdiff pixel
    SDL_Rect src = {0, 0, itf.win_width+xdiff, wave_height};
    SDL_Rect dst = {-xdiff, 0, itf.win_width+xdiff, wave_height};
    SDL_RenderCopy(itf.renderer, texture, &src, &dst);
  }
}

float get_scaled_left_sample(finyl_stem& s, int mindex) {
  auto left = s[mindex*2];
  return left / 32768.0;
}


void WaveForm::draw_wave(finyl_track& t, int x, int mindex) {
  if (t.stems_size == 1) {
    float sample = get_scaled_left_sample(*t.stems[0], mindex);
    int y = wave_height/2.0 - sample*wave_height_half;
    SDL_RenderDrawLine(itf.renderer, x, y, x, wave_height_half);
    return;
  }
  
  int amount0;
  {
    float sample = get_scaled_left_sample(*t.stems[0], mindex);
    amount0 = sample*wave_height_half;
    int y = wave_height_half - amount0;
    SDL_SetRenderDrawColor(itf.renderer, 255, 255, 255, 255); //white
    SDL_RenderDrawLine(itf.renderer, x, y, x, wave_height_half);
    SDL_SetRenderDrawColor(itf.renderer, 100, 100, 250, 255);
  }
  
  {
    float sample = get_scaled_left_sample(*t.stems[1], mindex);
    int amount1 = sample*wave_height_half;
    SDL_SetRenderDrawColor(itf.renderer, 100, 100, 250, 255);
    if ((amount0 > 0 && amount1 > 0) || (amount0<0 && amount1<0)) {
      int amount = amount0 + amount1;
      int y = wave_height_half - amount;
      int toy = wave_height_half - amount0;
      SDL_RenderDrawLine(itf.renderer, x, y, x, toy);
    } else {
      int y = wave_height_half - amount1;
      SDL_RenderDrawLine(itf.renderer, x, y, x, wave_height_half);
    }
  }
}

int get_pixel(int ioffset, int range, int window_width) {
  return (ioffset / (float)range) * window_width;
}


void WaveForm::draw_waveform(SDL_Texture* texture, finyl_track& t, int starti, int nowi, int draw_range) {
  SDL_SetRenderDrawColor(itf.renderer, 100, 100, 250, 255); // Waveform color

  int idraw_offset = wave_range-draw_range;
  int idraw_offset_end = wave_range;
  if (draw_range < 0) {
    idraw_offset = 0;
    idraw_offset_end = -draw_range;
  }
  
  int prev_pcmi = starti+idraw_offset;
  
  int beati = finyl_get_quantized_beat_index(t, prev_pcmi);
  if (beati != -1 && t.beats[beati].time * (gApp.audio->get_sample_rate() /1000.0) < prev_pcmi) {
    beati++;
  }

  for (int i = idraw_offset; i < idraw_offset_end; i=i+wave_iteration_margin) {
    int x = get_pixel(i, wave_range, itf.win_width);
    int pcmi = i+starti;
    
    //horizontal line
    bool has_pcm = pcmi>=0 && pcmi < t.get_refmsize();
    if (!has_pcm) {
      SDL_RenderDrawLine(itf.renderer, x, wave_height_half, x, wave_height_half);
      continue;
    }
    
    //beat grid
    if (beati != -1) {
      int beat_pcmi = t.beats[beati].time * (gApp.audio->get_sample_rate()/1000.0);
      if (prev_pcmi <= beat_pcmi && beat_pcmi < pcmi) {
        SDL_Rect rect = {x, 0, 0, wave_height};
        if (t.beats[beati].number == 1) {
          SDL_SetRenderDrawColor(itf.renderer, 255, 255, 255, 255);
          SDL_RenderFillRect(itf.renderer, &rect);
          SDL_SetRenderDrawColor(itf.renderer, 100, 100, 250, 255);
        } else if (wave_iteration_margin < 200) {
          SDL_SetRenderDrawColor(itf.renderer, 150, 150, 150, 255);
          SDL_RenderFillRect(itf.renderer, &rect);
          SDL_SetRenderDrawColor(itf.renderer, 100, 100, 250, 255);
        }
        beati++;
      }
    }
    
    //cue
    for (auto cue = t.cues.begin(); cue!=t.cues.end(); cue++) {
      auto cuei = cue->time*(gApp.audio->get_sample_rate()/ 1000.0);
      if (prev_pcmi < cuei  && cuei < pcmi) {
        SDL_SetRenderDrawColor(itf.renderer, 255, 71, 0, 255);
        SDL_RenderDrawLine(itf.renderer, x, 0, x, 10);
        SDL_SetRenderDrawColor(itf.renderer, 100, 100, 250, 255);
      }
    }
    
    draw_wave(t, x, pcmi);
    
    prev_pcmi = pcmi;
  }
}

int get_index(int win_width, int starti, int x, int range) {
  return starti + (int)((x / (float)win_width) * range);
}

int WaveForm::plus_waveform(SDL_Texture* texture, finyl_track& t, int nowi, int wave_y, int previ) {
  SDL_SetRenderTarget(itf.renderer, texture);
  int starti = nowi - (int)wave_range/2;
  int draw_range = starti - previ;
  int xdiff = (draw_range / (float)wave_range) * itf.win_width;
  int newprevi = get_index(itf.win_width, previ, xdiff, wave_range);
  
  slide(texture, xdiff);
  erase_edge(xdiff);
  
  draw_waveform(texture, t, starti, nowi, draw_range);
  
  SDL_SetRenderTarget(itf.renderer, NULL);
  SDL_Rect dst = {0, wave_y, itf.win_width, wave_height};
  SDL_RenderCopy(itf.renderer, texture, NULL, &dst);
  
  return newprevi;
}

void WaveForm::render_waveform(SDL_Texture* texture, finyl_track& t, int nowi, int wave_y) {
  SDL_SetRenderTarget(itf.renderer, texture);
  int starti = nowi - (int)wave_range/2;

  SDL_SetRenderDrawColor(itf.renderer, 0, 0, 0, 0);
  SDL_RenderClear(itf.renderer);
  
  draw_waveform(texture, t, starti, nowi, wave_range);
  
  SDL_SetRenderTarget(itf.renderer, NULL);
  SDL_Rect dst = {0, wave_y, itf.win_width, wave_height};
  SDL_RenderCopy(itf.renderer, texture, NULL, &dst);
}

void WaveForm::draw_center_line() {
  SDL_SetRenderDrawColor(itf.renderer, 255, 0, 250, 255); // Waveform color
  SDL_Rect rect = {itf.win_width / 2 - 1, 0, 2, wave_height};
  SDL_RenderFillRect(itf.renderer, &rect);
}

void WaveForm::draw_static_grids(finyl_track* t) {
  if (t->beats.size() < 2) {
    return;
  }
  
  SDL_SetRenderDrawColor(itf.renderer, 100, 0, 100, 255);

  int dur = t->beats[1].time - t->beats[0].time; //msec
  int samples = dur * (gApp.audio->get_sample_rate() / 1000.0);

  int i = 1;
  while (1) {
    int xl = get_pixel(wave_range/2 - 32*i*samples, wave_range, itf.win_width);
    if (xl>itf.win_width || xl<0) {
      break;
    }
    SDL_Rect rectl = {xl-1, 0, 2, wave_height};
    SDL_RenderFillRect(itf.renderer, &rectl);
    
    int xr = get_pixel(wave_range/2 + 32*i*samples, wave_range, itf.win_width);
    
    SDL_Rect rectr = {xr-1, 0, 2, wave_height};
    SDL_RenderFillRect(itf.renderer, &rectr);
    i++;

  }
}

void WaveForm::render_static_grids(SDL_Texture* texture, finyl_track* t, int wave_y) {
  SDL_SetRenderTarget(itf.renderer, texture);
  
  SDL_SetRenderDrawColor(itf.renderer, 0, 0, 0, 0);
  SDL_RenderClear(itf.renderer);

  draw_center_line();
  draw_static_grids(t);
  
  SDL_SetRenderTarget(itf.renderer, NULL);
  SDL_Rect dst = {0, wave_y, itf.win_width, wave_height};
  SDL_RenderCopy(itf.renderer, texture, NULL, &dst);
}

void WaveForm::render_deck(Deck& deck, int& previ, SDL_Texture* wavetx, int wave_y) {
  int nowi = (int)deck.pTrack->get_refindex();
  render_waveform(wavetx, *deck.pTrack, nowi, wave_y);
  previ = nowi - (int)wave_range/2;
}

void WaveForm::update_deck(Deck &deck, int &previ, SDL_Texture* wavetx, SDL_Texture* sgtx, int wave_y) {
  previ = plus_waveform(wavetx, *deck.pTrack, (int)deck.pTrack->get_refindex(), wave_y, previ);
  render_static_grids(sgtx, deck.pTrack, wave_y);
}

void WaveForm::draw() {
  if (render_adeck) {
    render_deck(*gApp.controller->adeck, previ_adeck, tx_awave, 0);
    render_adeck = false;
  } else if (gApp.controller->adeck->pTrack != nullptr) {
    update_deck(*gApp.controller->adeck, previ_adeck, tx_awave, tx_asg, 0);
  }
    
  if (render_bdeck) {
    render_deck(*gApp.controller->bdeck, previ_bdeck, tx_bwave, wave_height + 10);
    render_bdeck = false;
  } else if (gApp.controller->bdeck->pTrack != nullptr) {
    update_deck(*gApp.controller->bdeck, previ_bdeck, tx_bwave, tx_bsg, wave_height + 10);
  }
}
