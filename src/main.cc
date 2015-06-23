/*
 * The MIT License (MIT)
 *
 * Copyright © 2015 Allen Goodman
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the “Software”),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED “AS IS,” WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "veranke.h"

#include <fstream>

#include <SDL2/SDL.h>

static const int SCREEN_WIDTH  = 64;
static const int SCREEN_HEIGHT = 32;

static const std::uint8_t SCALE = 10;

static SDL_Window * window;

static SDL_Surface * surface;

static SDL_Event event;

static SDL_Renderer * renderer;

static SDL_Keycode keymap[16] = {
  SDLK_1,
  SDLK_2,
  SDLK_3,
  SDLK_4,
  SDLK_q,
  SDLK_w,
  SDLK_e,
  SDLK_r,
  SDLK_a,
  SDLK_s,
  SDLK_d,
  SDLK_f,
  SDLK_z,
  SDLK_x,
  SDLK_c,
  SDLK_v
};

static int events(Veranke &veranke) {
  veranke.keypad.fill(0);

  SDL_PollEvent(&event);

  switch (event.type) {
    case SDL_KEYDOWN:
      for (std::size_t i = 0; i < 16; i++) {
        if (keymap[i] == event.key.keysym.sym) {
          veranke.keypad[i] = 1;

          break;
        }
      }

      return 1;

    case SDL_QUIT:
      return 0;

    default:
      return 1;
  }
}

int main(int argc, char **argv) {
  if (argc > 1) {
    Veranke veranke;

    std::ifstream file;

    std::size_t size;

    file.open(argv[1], std::ios_base::in | std::ios_base::binary);

    if (file.is_open()) {
      file.seekg(0, std::ios_base::end);

      size = (std::size_t) file.tellg();

      if (size <= 0xFFF - 0x200) {
        char * ROM = (char *) (&(veranke.memory[0x200]));

        file.seekg(0, std::ios_base::beg);

        file.read(ROM, size);
      }

      file.close();
    }

    SDL_Init(SDL_INIT_VIDEO);

    surface = SDL_CreateRGBSurface(0, 10, 10, 32, 0, 0, 0, 0);

    SDL_FillRect(surface, NULL, SDL_MapRGB(surface->format, 255, 255, 255));

    window = SDL_CreateWindow("…", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 320, SDL_WINDOW_RESIZABLE);

    SDL_Surface * screen = SDL_GetWindowSurface(window);

    renderer = SDL_CreateRenderer(window, -1, 0);

    auto events_result = events(veranke);

    while (events_result) {
      veranke.decode_and_execute();

      if (veranke.delay_timer > 0) {
        --veranke.delay_timer;
      }

      if (veranke.sound_timer > 0) {
        // TODO: implement beep
        --veranke.sound_timer;
      }

      SDL_FillRect(screen, NULL, SDL_MapRGB(screen-> format, 0, 0, 0));

      SDL_Rect position;

      for (size_t x = 0; x < SCREEN_WIDTH; x++) {
        for (size_t y = 0; y < SCREEN_HEIGHT; y++) {
          position.x = (int) (x * SCALE);
          position.y = (int) (y * SCALE);
          position.w = SCALE;
          position.h = SCALE;

          if (veranke.video_memory[y * 64 + x] == 1) {
            SDL_FillRect(screen, &position, SDL_MapRGB(screen->format, 255, 255, 255));
          } else {
            SDL_FillRect(screen, &position, SDL_MapRGB(screen->format, 0, 0, 0));
          }
        }
      }

      SDL_UpdateWindowSurface(window);
    }

    SDL_FreeSurface(surface);

    SDL_Quit();
  }

  return 0;
}
