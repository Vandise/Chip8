#include <iostream>
#include "processor/chip8.hpp"

using namespace std;

int
main( const int argc, const char **argv )
{
  if (argc != 2) {
    cout << "Usage: c8 <ROM file>" << endl;
    return 1;
  }

  Processor::Chip8 C8(argv[1]);
  C8.initialize();
  C8.dumpMemory();

  return 0;
}