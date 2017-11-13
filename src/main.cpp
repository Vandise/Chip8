#include <iostream>
#include <chrono>
#include <thread>
#include "display/screen.hpp"
#include "processor/chip8.hpp"

using namespace std;

#define LD(byte) c8->memory[512 + offset] = byte; offset++;

void
chip8Debug(Processor::Chip8 *c8, Display::Screen *window)
{
  for (int i = 0; i < 4096; ++i)
  {
    c8->memory[i] = 0;
  }
  int offset = 0;

  // sprites are max 8px wide
  // print out stuff

  // ld_vx_byte
  // - set register 1 to 09 (the single digit hex value to print)
    LD(0x61); LD(0x00);

  // fx_ld_f_vx
  // - index register points to register 1 (contains the sprite we want to load)
    LD(0xF1); LD(0x29);

  // ld_vx_byte
  // - set register 2 to 10 ( the x position )
    LD(0x62); LD(0x10);

  // ld_vx_byte
  // - set register 3 to 5 ( the y position )
    LD(0x63); LD(0x05);

  // drw_vx_vy_nibble
  // - draw x value in reg 2, y value in reg 3, the number of pixels ( 5 )
  // - index pointer contains register value
    LD(0xD2); LD(0x35);

  // jp_addr
    LD(0x12); LD(0x0A);

}

int
main( const int argc, const char **argv )
{
  if (argc < 2) {
    cout << "Usage: c8 <ROM file>" << endl;
    return 1;
  }

  Display::Screen *window = new Display::Screen();
  Processor::Chip8 *C8 = new Processor::Chip8(argv[1]);

  C8->initialize();

  if (argc == 3)
  {
    chip8Debug(C8, window);
    C8->debugMemory();

    while (true)
    {
      C8->cycle();
      window->inputManager();

      if (C8->drawFlag)
      {
        C8->drawFlag = false;
        for ( int i = 0; i < 2048; ++i)
        {
          uint8_t pixel = C8->graphicsBuffer[i];
          window->pushToBuffer(i, ((0x00FFFFFF * pixel) | 0xFF000000));
        }

        window->refresh();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }
    }

    delete(C8);
    delete(window);
    return 0;
  }

  while (true)
  {
    C8->cycle();

    // pull events (key presses)
    window->inputManager();

    //
    // drawing
    //
    if (C8->drawFlag)
    {
      C8->drawFlag = false;
      //window->clearPixelBuffer();
      for ( int i = 0; i < 2048; ++i)
      {
        uint8_t pixel = C8->graphicsBuffer[i];
        window->pushToBuffer(i, ((0x00FFFFFF * pixel) | 0xFF000000));
      }

      window->refresh();
      std::this_thread::sleep_for(std::chrono::milliseconds(75));
    }

  }
  delete(C8);
  delete(window);
  return 0;
}