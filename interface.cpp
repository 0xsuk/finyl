#include <SDL2/SDL.h>
#include <math.h>
#include "finyl.h"
#include "interface.h"
#include <X11/Xlib.h>

int get_window_size(int& width, int& height) {
  Display* disp = XOpenDisplay(NULL);
  if (disp == NULL) {
    fprintf(stderr, "Unable to connect to display\n");
    return -1;
  }

  int screenNum = DefaultScreen(disp);
  width = DisplayWidth(disp, screenNum);
  height = DisplayHeight(disp, screenNum);

  XCloseDisplay(disp);
  return 0;
}

Interface::Interface(): fps(33) {
  
}

void Interface::add_track_to_free(finyl_track* t) {
  tracks_to_free[tracks_to_free_tail] = t;
  tracks_to_free_tail++;
}

void Interface::free_tracks() {
  for (int i = 0; i<tracks_to_free_tail; i++) {
    delete tracks_to_free[i];
  }
  tracks_to_free_tail = 0;
}


int Interface::run() {
  if (get_window_size(win_width, win_height) == -1) {
    return 1;
  }
  
  win_width = 1500;
  win_height = 1000;
  
  if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
    printf("SDL initialization failed: %s\n", SDL_GetError());
    return 1;
  }
  
  if ( TTF_Init() < 0 ) {
		printf("Error intializing SDL_ttf: %s", TTF_GetError());
		return 1;
	}

  window = SDL_CreateWindow("finyl", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, win_width, win_height, SDL_WINDOW_SHOWN);
  if (window == nullptr) {
    printf("Window creation failed: %s\n", SDL_GetError());
    return 1;
  }

  SDL_SetWindowOpacity(window, 0.8);
  
  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
  if (renderer == nullptr) {
    printf("Renderer creation failed: %s\n", SDL_GetError());
    return 1;
  }

  waveform = std::make_unique<Waveform>();
  explorer = std::make_unique<Explorer>();
  cat = std::make_unique<GIF>("assets/dance-cat.gif", 0, win_height - 350, 250, 250);
  pikachu = std::make_unique<GIF>("assets/pikachu-pokemon.gif", win_width - 350 , win_height - 400, 400, 300);
  a_spectrum = std::make_unique<Spectrum>(*gApp.controller->adeck->fftState);
  // a_spectrum = std::make_unique<Spectrum>(*gApp.controller->adeck->fftState);
  
  SDL_Event event;
  int desired_delta = 1000 / fps;
  while (gApp.is_running()) {
    int start_msec = SDL_GetTicks();
    
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        goto cleanup;
      }
      if (event.type == SDL_KEYDOWN) {
        gApp.controller->handle_sdl_key(event);
      }
    }

    SDL_SetRenderDrawColor(renderer, 0,0,0,255);
    SDL_RenderClear(renderer);

    free_tracks();
    
    waveform->draw();
    a_spectrum->draw();
    explorer->draw();
    cat->draw();
    pikachu->draw();
        
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
