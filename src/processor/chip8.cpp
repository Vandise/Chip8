#include "processor/chip8.hpp"

#include <stdio.h>
#include <stdlib.h>

Processor::Chip8::Chip8(const char *file_path)
{
  this->filename = file_path;

  // our program is loaded at "address" 0x200
  this->programCounter = C8_MEMORY_OFFSET_HEX;

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

  // Instruction Handlers
  this->instructions[0x6000] = &Chip8::ld_vx_byte;
}

void
Processor::Chip8::dumpMemory()
{
  std::cout << hexdump(this->memory) << std::endl;
}

void
Processor::Chip8::cycle()
{

  this->opCode = this->memory[this->programCounter] << 8 | this->memory[this->programCounter + 1];

  /*
    Instruction Example:
      0x6a02

    Big Endian:
      low ---> high
        02   6a

    Mask:
      0x026a & 0xF000
          ^      ^
          |------|
      0x026a & 0xF000
        ^          ^
        |----------|  
  */

  uint16_t instruction = this->opCode & 0xF000;
  if ( this->instructions.count(instruction) )
  {
    (this->*(this->instructions[instruction]))();
  }
  else
  {
    std::cout << "Unimplemented OpCode: " << hexdump(this->opCode) << std::endl;
    exit(1);
  }

}

/*
  6xkk - LD Vx, byte
    Set Vx = kk.
    The interpreter puts the value kk into register Vx.
*/
void
Processor::Chip8::ld_vx_byte()
{
  std::cout << "ld_vx_byte: " << hexdump(this->opCode) << std::endl;
}