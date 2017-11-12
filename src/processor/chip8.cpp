#include "processor/chip8.hpp"
#include "processor/fontset.hpp"

Processor::Chip8::Chip8(const char *file_path)
{
  this->filename = file_path;
  this->drawFlag = false;

  // our program is loaded at "address" 0x200
  this->programCounter = C8_MEMORY_OFFSET_HEX;
  this->sp = 0;
  this->delayTimer = 0;
  this->soundTimer = 0;

  for (int i = 0; i < 4096; ++i)
  {
    memory[i] = 0;
  }

  for (int i = 0; i < 2048; ++i) {
    this->graphicsBuffer[i] = 0;
  }

  for (int i = 0; i < 16; ++i) {
    this->registers[i] = 0;
    this->stack[i] = 0;
  }

  for (int i = 0; i < 80; ++i) {
    this->memory[i] = chip8_fontset[i];
  }

  srand (time(NULL));
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
  this->instructions[0x0000] = &Chip8::return_clear_screen;
  this->instructions[0x1000] = &Chip8::jp_addr;
  this->instructions[0x2000] = &Chip8::call_addr;
  this->instructions[0x3000] = &Chip8::se_vx_byte;
  this->instructions[0x6000] = &Chip8::ld_vx_byte;
  this->instructions[0x7000] = &Chip8::add_vx_byte;
  this->instructions[0xA000] = &Chip8::ld_i_addr;
  this->instructions[0xD000] = &Chip8::drw_vx_vy_nibble;
  this->instructions[0xF000] = &Chip8::fx_entrance;
    // 0xF instructions
    this->fxInstructions[0x0007] = &Chip8::fx_ld_vx_dt;
    this->fxInstructions[0x0015] = &Chip8::fx_ld_dt_vx;
    this->fxInstructions[0x0029] = &Chip8::fx_ld_vx_i;
    this->fxInstructions[0x0033] = &Chip8::fx_ld_b_vx;
    this->fxInstructions[0x0065] = &Chip8::fx_ld_f_vx;
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
  }

  if (this->delayTimer > 0)
  {
    --this->delayTimer;
  }

  if (this->soundTimer > 0)
  {
    if (this->soundTimer == 1)
    {
      // BEEP!
    }
    --this->soundTimer;
  }
}

/*
  0x0000
*/
void
Processor::Chip8::return_clear_screen()
{
  std::cout << "return_clear_screen: " << hexdump(this->opCode) << std::endl;

  switch( this->opCode & 0x000F)
  {
    // clear screen
    case 0x0000:
        for (int i = 0; i < 2048; ++i) {
          this->graphicsBuffer[i] = 0;
        }
        this->drawFlag = true;
        this->programCounter += 2;
      break;
    // return from subroutine
    case 0x000E:
      --this->sp;
      this->programCounter = this->stack[this->sp];
      this->programCounter += 2;
      break;
    default:
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
  for ( int yline = 0; yline < height; yline++ )
  {
    // fetch each pixel value starting from the indexRegister
    pixel = this->memory[this->indexRegister + yline];

    // the width is 8 pixels
    for ( int xline = 0; xline < 8; xline++ )
    {
      // scan, one byte at a time, if the current EVALUATED pixel is set to 1
      if ( (pixel & (0x80 >> xline)) != 0 ) // 0x80 = 128
      {
        // check if the current pixel on display is set to 1,
        // if so, we register the collision by setting register (V)F to 1
        //int offset = (xCoord + y + ((yCoord + w) * C8_GFX_LENGTH));
        int offset = (xCoord + xline + ((yCoord + yline) * C8_GFX_LENGTH));
        if ( this->graphicsBuffer[(xCoord + xline + ((yCoord + yline) * C8_GFX_LENGTH))] == 1 )
        {
          this->registers[0xF] = 1;
        }
        // XOR the pixel to redraw the screen
        this->graphicsBuffer[(xCoord + xline + ((yCoord + yline) * C8_GFX_LENGTH))] ^= 1;
      }
    }
  }
  // redraw the screen
  this->drawFlag = true;
  this->programCounter += 2;
}

/*
  1nnn - JP addr
    Jump to location nnn.

    The interpreter sets the program counter to nnn.
*/
void
Processor::Chip8::jp_addr()
{
  std::cout << "jp_addr: " << hexdump(this->opCode) << std::endl;
  this->programCounter = this->opCode & 0x0FFF;
}

/*
  2nnn - CALL addr
    Call subroutine at nnn.
    The interpreter increments the stack pointer, then puts the current PC on the top of the stack. The PC is then set to nnn.
*/
void
Processor::Chip8::call_addr()
{
  std::cout << "call_addr: " << hexdump(this->opCode) << std::endl;

  this->stack[this->sp] = this->programCounter;
  ++this->sp;
  this->programCounter = this->opCode & 0x0FFF;
}

/*
  3xkk - SE Vx, byte
    Skip next instruction if Vx = kk.

    The interpreter compares register Vx to kk, and if they are equal, increments the program counter by 2.
*/
void
Processor::Chip8::se_vx_byte()
{
  if ( this->registers[(this->opCode & 0x0F00) >> 8] == (this->opCode & 0x00FF) )
  {
    this->programCounter += 4;
  }
  else
  {
    this->programCounter += 2;
  }
}

/*
  Handles FxNN instruction mapping
*/
void
Processor::Chip8::fx_entrance()
{
  uint16_t instruction = this->opCode & 0x00FF;
  if ( this->fxInstructions.count(instruction) )
  {
    (this->*(this->fxInstructions[instruction]))();
  }
  else
  {
    std::cout << "Unimplemented FX OpCode: " << hexdump(this->opCode) << std::endl;
  }
}

/*
  Fx07 - LD Vx, DT
    Set Vx = delay timer value.

    The value of DT is placed into Vx.
*/
void
Processor::Chip8::fx_ld_vx_dt()
{
  std::cout << "fx_ld_vx_dt: " << hexdump(this->opCode) << std::endl;

  this->registers[(this->opCode & 0x0F00) >> 8] = this->delayTimer;
  this->programCounter += 2;
}

/*
  Fx15 - LD DT, Vx
    Set delay timer = Vx.

    DT is set equal to the value of Vx.
*/
void
Processor::Chip8::fx_ld_dt_vx()
{
  std::cout << "fx_ld_dt_vx: " << hexdump(this->opCode) << std::endl;

  this->delayTimer = this->registers[(this->opCode & 0x0F00) >> 8];
  this->programCounter += 2;
}

/*
Fx33 - LD B, Vx
  Store BCD representation of Vx in memory locations I, I+1, and I+2.

  The interpreter takes the decimal value of Vx, and places the hundreds digit in memory at location in I, 
  the tens digit at location I+1, and the ones digit at location I+2.
*/
void
Processor::Chip8::fx_ld_b_vx()
{
  std::cout << "fx_ld_b_vx: " << hexdump(this->opCode) << std::endl;

  this->memory[this->indexRegister]     = (this->registers[(this->opCode & 0x0F00) >> 8]) / 100;
  this->memory[this->indexRegister + 1] = ((this->registers[(this->opCode & 0x0F00) >> 8]) / 10) % 10;
  this->memory[this->indexRegister + 2] = ((this->registers[(this->opCode & 0x0F00) >> 8]) % 100) % 10;

  this->programCounter += 2;
}

/*
  Fx65 - LD Vx, [I]
    Read registers V0 through Vx from memory starting at location I.

    The interpreter reads values from memory starting at location I into registers V0 through Vx.
*/
void
Processor::Chip8::fx_ld_vx_i()
{
  std::cout << "fx_ld_vx_i: " << hexdump(this->opCode) << std::endl;

  for (int i = 0; i <= ((this->opCode & 0x0F00) >> 8); ++i)
  {
    this->registers[i] = this->memory[this->indexRegister + i];
  }
  this->indexRegister += ((this->opCode & 0x0F00) >> 8) + 1;
  this->programCounter += 2;
}

/*
  Fx29 - LD F, Vx
    Set I = location of sprite for digit Vx.

    The value of I is set to the location for the hexadecimal sprite corresponding to the value of Vx
*/
void
Processor::Chip8::fx_ld_f_vx()
{
  std::cout << "fx_ld_f_vx: " << hexdump(this->opCode) << std::endl;

  this->indexRegister = this->registers[(this->opCode & 0x0F00) >> 8] * 0x5;
  this->programCounter += 2;
}

/*
  7xkk - ADD Vx, byte
    Set Vx = Vx + kk.
    Adds the value kk to the value of register Vx, then stores the result in Vx. 
*/
void
Processor::Chip8::add_vx_byte()
{
  std::cout << "add_vx_byte: " << hexdump(this->opCode) << std::endl;

  this->registers[(this->opCode & 0x0F00) >> 8] += (this->opCode & 0x00FF);
  this->programCounter += 2;
}


