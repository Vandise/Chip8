#ifndef __DISPLAY_SCREEN
#define __DISPLAY_SCREEN 1

#include "SDL2/SDL.h"
#include <iostream>

#define WINDOW_HEIGHT 512
#define WINDOW_WIDTH 1024
#define PIXEL_BUFFER_SIZE 2048

namespace Display
{

  class Screen
  {

    private:
      SDL_Event e;
      SDL_Window* window = NULL;
      SDL_Renderer *renderer;
      SDL_Texture* sdlTexture;
      uint32_t pixelBuffer[PIXEL_BUFFER_SIZE];

    public:
      Screen();
      ~Screen();
      void clearPixelBuffer();
      void pushToBuffer(int index, uint32_t pixel);
      void refresh();
      void inputManager();

  };

}

#endif