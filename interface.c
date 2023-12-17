#include <SDL2/SDL.h>
#include <math.h>
#include "finyl.h"

#define WINDOW_WIDTH 1600
#define WINDOW_HEIGHT 100
#define SAMPLE_RATE 44100

float get_scaled_sample(finyl_channel c, int position) {
    float y = finyl_get_sample1(c, position) / 32768.0;
    return y;
}

void render_waveform(SDL_Renderer *renderer, finyl_track* t) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Background color
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 100, 100, 250, 255); // Waveform color

    for (int i = 0; i < t->length; i++) {
      int x = ((i - floor(t->index)) / (float)t->length) * WINDOW_WIDTH;
      if (x < 0 || x >= WINDOW_WIDTH) continue;
      
      int y = WINDOW_HEIGHT / 2.0 - get_scaled_sample(t->channels[0], i) * (WINDOW_HEIGHT / 2.0);
      SDL_RenderDrawPoint(renderer, x, y);
    }

    SDL_RenderPresent(renderer);
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

    int scroll_position = 0;
    int scroll_speed = 1; // Adjust this to change the scrolling speed

    SDL_Event event;
    while (finyl_running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                finyl_running = false;
            }
        }

        render_waveform(renderer, t);

        SDL_Delay(16); // Approximately 60 frames per second
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
