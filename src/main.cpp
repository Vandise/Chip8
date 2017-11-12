#include <iostream>
#include <chrono>
#include <thread>
#include "display/screen.hpp"
#include "processor/chip8.hpp"

using namespace std;

int
main( const int argc, const char **argv )
{
  if (argc != 2) {
    cout << "Usage: c8 <ROM file>" << endl;
    return 1;
  }

  Display::Screen *window = new Display::Screen();
  Processor::Chip8 *C8 = new Processor::Chip8(argv[1]);
  C8->initialize();

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
      for ( int i = 0; i < 2048; ++i)
      {
        uint8_t pixel = C8->graphicsBuffer[i];
        window->pushToBuffer(i, ((0x00FFFFFF * pixel) | 0xFF000000));
      }
      window->refresh();
    }
    std::this_thread::sleep_for(std::chrono::microseconds(2400));

  }
  delete(C8);
  delete(window);
  return 0;
}