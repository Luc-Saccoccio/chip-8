#include <SDL2/SDL.h>
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_surface.h>
#include <SDL2/SDL_video.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "chip8.h"

#define COLS 64
#define ROWS 32
#define SCALE 10

SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;
SDL_Event event;

void usage(void) { fputs("chip-8 [file]\n", stderr); }

void unload_SDL(void) {
  if (renderer != NULL)
    SDL_DestroyRenderer(renderer);
  if (window != NULL)
    SDL_DestroyWindow(window);
  SDL_Quit();
}

void error_happened(const char *function) {
  fprintf(stderr, "Error with %s: %s", function, SDL_GetError());
  unload_SDL();
  exit(EXIT_FAILURE);
}

void draw_display(void) {
  SDL_Rect rect = {.x = 0, .y = 0, .h = 64 * SCALE, .w = 64 * SCALE};
  for (int i = 0; i < 64; i++)
    for (int j = 0; j < 32; j++) {
      rect.x = i * SCALE;
      rect.y = j * SCALE;
      rect.w = SCALE;
      rect.h = SCALE;
      if (display[i + (j * 64)] == 1)
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
      else
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
      SDL_RenderFillRect(renderer, &rect);
    }
  SDL_RenderPresent(renderer);
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    usage();
    return EXIT_FAILURE;
  }
  vm_init();
  // TODO: Narrow down the "SDL_INIT_EVERYTHING"
  SDL_Init(SDL_INIT_EVERYTHING);
  // TODO: Name of ROM in window name
  window =
      SDL_CreateWindow("chip-8: [name]", SDL_WINDOWPOS_CENTERED,
                       SDL_WINDOWPOS_CENTERED, COLS * SCALE, ROWS * SCALE, 0);
  if (window == NULL)
    error_happened("SDL_CreateWindow");
  renderer = SDL_CreateRenderer(window, -1, 0);
  if (renderer == NULL)
    error_happened("SDL_CreateRenderer");
  if (0 != load_program(argv[1])) {
    unload_SDL();
    return EXIT_FAILURE;
  }
  uint16_t done = 0;
  while (done == 0) {
    while (SDL_PollEvent(&event))
      switch (event.type) {
      case SDL_KEYDOWN:
        switch (event.key.keysym.sym) {
        case SDLK_1:
          key[0x1] = 1;
          break;
        case SDLK_2:
          key[0x2] = 1;
          break;
        case SDLK_3:
          key[0x3] = 1;
          break;
        case SDLK_4:
          key[0xC] = 1;
          break;
        case SDLK_q:
          key[0x4] = 1;
          break;
        case SDLK_w:
          key[0x5] = 1;
          break;
        case SDLK_e:
          key[0x6] = 1;
          break;
        case SDLK_r:
          key[0xD] = 1;
          break;
        case SDLK_a:
          key[0x7] = 1;
          break;
        case SDLK_s:
          key[0x8] = 1;
          break;
        case SDLK_d:
          key[0x9] = 1;
          break;
        case SDLK_f:
          key[0xE] = 1;
          break;
        case SDLK_z:
          key[0xA] = 1;
          break;
        case SDLK_x:
          key[0x0] = 1;
          break;
        case SDLK_c:
          key[0xB] = 1;
          break;
        case SDLK_v:
          key[0xF] = 1;
          break;
        case SDLK_ESCAPE:
          exit(1);
          break;
        }
        break;
      case SDL_KEYUP:
        switch (event.key.keysym.sym) {
        case SDLK_1:
          key[0x1] = 0;
          break;
        case SDLK_2:
          key[0x2] = 0;
          break;
        case SDLK_3:
          key[0x3] = 0;
          break;
        case SDLK_4:
          key[0xC] = 0;
          break;
        case SDLK_q:
          key[0x4] = 0;
          break;
        case SDLK_w:
          key[0x5] = 0;
          break;
        case SDLK_e:
          key[0x6] = 0;
          break;
        case SDLK_r:
          key[0xD] = 0;
          break;
        case SDLK_a:
          key[0x7] = 0;
          break;
        case SDLK_s:
          key[0x8] = 0;
          break;
        case SDLK_d:
          key[0x9] = 0;
          break;
        case SDLK_f:
          key[0xE] = 0;
          break;
        case SDLK_z:
          key[0xA] = 0;
          break;
        case SDLK_x:
          key[0x0] = 0;
          break;
        case SDLK_c:
          key[0xB] = 0;
          break;
        case SDLK_v:
          key[0xF] = 0;
          break;
        }
        break;
      case SDL_QUIT:
        done = 1;
        break;
      default:
        break;
      }
    vm_cycle();
    if (draw != 0) {
      fputs("Display drawn\n", stderr);
      draw_display();
      draw = 0;
    }
  }
  unload_SDL();
  return EXIT_SUCCESS;
}
