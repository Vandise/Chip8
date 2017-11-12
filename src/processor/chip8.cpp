#include "processor/chip8.hpp"

#include <stdio.h>
#include <stdlib.h>

Processor::Chip8::Chip8(const char *file_path)
{
  this->filename = file_path;
  this->drawFlag = false;

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
  this->instructions[0xA000] = &Chip8::ld_i_addr;
  this->instructions[0xD000] = &Chip8::drw_vx_vy_nibble;
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
    std::this_thread::sleep_for(std::chrono::microseconds(C8_EMULATION_SPEED_SLEEP));
  }
  else
  {
    std::cout << "Unimplemented OpCode: " << hexdump(this->opCode) << std::endl;
    //exit(1);
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

  this->registers[(this->opCode & 0x0F00)] = (this->opCode & 0x00FF);
  this->programCounter += 2;
}

/*
  Annn - LD I, addr
    Set I = nnn.
    The value of register I is set to nnn.
*/
void
Processor::Chip8::ld_i_addr()
{
  std::cout << "ld_i_addr: " << hexdump(this->opCode) << std::endl;

  this->indexRegister = (this->opCode & 0x0FFF);
  this->programCounter += 2;
}

/*
  Dxyn - DRW Vx, Vy, nibble
    x - x coordinate
    y - y coordinate
    n - number of pixels (height)
    width: 8 pixels
  
    Display n-byte sprite starting at memory location I at (Vx, Vy), set VF = collision.
    The interpreter reads n bytes from memory, starting at the address stored in I. These bytes are then displayed as sprites on screen
    at coordinates (Vx, Vy). Sprites are XORed onto the existing screen. If this causes any pixels to be erased, VF is set to 1,
    otherwise it is set to 0. If the sprite is positioned so part of it is outside the coordinates of the display, it wraps around to the
    opposite side of the screen
*/
void
Processor::Chip8::drw_vx_vy_nibble()
{
  std::cout << "drw_vx_vy_nibble: " << hexdump(this->opCode) << std::endl;

  unsigned short xCoord = this->registers[(this->opCode & 0x0F00) >> 8];
  unsigned short yCoord = this->registers[(this->opCode & 0x00F0) >> 4];
  unsigned short height = this->opCode & 0x000F;
  unsigned short pixel;

  // reset register (V)F to 0 as nothing is erased (yet)
  this->registers[0xF] = 0;

  // draw vertical (y line)
  for ( int y = 0; y < height; y++ )
  {
    // fetch each pixel value starting from the indexRegister
    pixel = this->memory[this->indexRegister + y];

    // the width is 8 pixels
    for ( int w = 0; w < 8; w++ )
    {
      // scan, one byte at a time, if the current EVALUATED pixel is set to 1
      if ( (pixel & (0x80 >> w)) != 0 ) // 0x80 = 128
      {
        // check if the current pixel on display is set to 1,
        // if so, we register the collision by setting register (V)F to 1
        int offset = (xCoord + y + ((yCoord + w) * C8_GFX_LENGTH));
        if ( this->graphicsBuffer[offset] == 1 )
        {
          this->registers[0xF] = 1;
        }
        // XOR the pixel to redraw the screen
        this->graphicsBuffer[offset] ^= 1;
      }
    }
  }
  // redraw the screen
  this->drawFlag = true;
  this->programCounter += 2;
}






















