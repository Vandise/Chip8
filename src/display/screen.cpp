#include "display/screen.hpp"

Display::Screen::Screen()
{
  this->window = SDL_CreateWindow(
    "CHIP-8",
    SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
    WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN
  );

  if (this->window == NULL)
  {
    printf( "Window could not be created! SDL_Error: %s\n", SDL_GetError() );
    exit(1);
  }

  this->renderer = SDL_CreateRenderer(window, -1, 0);
  SDL_RenderSetLogicalSize(this->renderer, WINDOW_WIDTH, WINDOW_HEIGHT);

  this->sdlTexture = SDL_CreateTexture(renderer,
    SDL_PIXELFORMAT_ARGB8888,
    SDL_TEXTUREACCESS_STREAMING,
    64, 32);
}

Display::Screen::~Screen()
{
  SDL_Quit();
}

void
Display::Screen::pushToBuffer(int index, uint32_t pixel)
{
  this->pixelBuffer[index] = pixel;
}

void
Display::Screen::inputManager()
{
  while(SDL_PollEvent(&(this->e)))
  {
    if (e.type == SDL_QUIT) exit(0);
  }
}

void
Display::Screen::refresh()
{

  // Update SDL texture
  SDL_UpdateTexture(this->sdlTexture, NULL, this->pixelBuffer, 64 * sizeof(Uint32));

  // Clear screen and render
  SDL_RenderClear(this->renderer);
  SDL_RenderCopy(this->renderer, this->sdlTexture, NULL, NULL);
  SDL_RenderPresent(this->renderer);
}