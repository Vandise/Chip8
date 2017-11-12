#include "processor/chip8.hpp"

#include <stdio.h>
#include <stdlib.h>

Processor::Chip8::Chip8(const char *file_path)
{
  this->filename = file_path;
  for (int i = 0; i < 4096; ++i)
  {
    memory[i] = 0;
  }
}

void
Processor::Chip8::initialize()
{
  char byte;
  int i = 0;
  this->file = std::ifstream(this->filename, std::ios::binary);
  while( this->file.get(byte) )
  {
    this->memory[i + C8_MEMORY_OFFSET] = (uint8_t)byte;
    i++;
  }
  std::cout << hexdump(this->memory);
}