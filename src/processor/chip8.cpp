#include "processor/chip8.hpp"
#include "processor/fontset.hpp"

Processor::Chip8::Chip8(const char *file_path)
{
  this->filename = file_path;
  //this->drawFlag = false;

  // our program is loaded at "address" 0x200
  this->programCounter = C8_MEMORY_OFFSET_HEX;
  this->sp = 0;
  this->delayTimer = 0;
  this->soundTimer = 0;

  for (int i = 0; i < 4096; ++i)
  {
    this->memory[i] = 0;
  }

  for (int i = 0; i < 2048; ++i) {
    this->graphicsBuffer[i] = 0;
  }

  for (int i = 0; i < 16; ++i) {
    this->registers[i] = 0;
    this->stack[i] = 0;
    this->key[i] = 0;
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

  for (int i = 0; i < 80; ++i)
  {
    this->memory[i] = chip8_fontset[i];
  }

  // Instruction Handlers
  this->instructions[0x0000] = &Chip8::return_clear_screen;       // validated
  this->instructions[0x1000] = &Chip8::jp_addr;                   // validated
  this->instructions[0x2000] = &Chip8::call_addr;                 // validated
  this->instructions[0x3000] = &Chip8::se_vx_byte;                // validated
  this->instructions[0x4000] = &Chip8::sne_vx_byte;               // validated
  this->instructions[0x6000] = &Chip8::ld_vx_byte;                // validated
  this->instructions[0x7000] = &Chip8::add_vx_byte;               // validated
  this->instructions[0x8000] = &Chip8::register_vx_vy_byte;       // validated
  this->instructions[0xA000] = &Chip8::ld_i_addr;
  this->instructions[0xC000] = &Chip8::rnd_vx_byte;
  this->instructions[0xD000] = &Chip8::drw_vx_vy_nibble;
  this->instructions[0xE000] = &Chip8::ex_skip;
  this->instructions[0xF000] = &Chip8::fx_entrance;
    // 0xF instructions
    this->fxInstructions[0x0007] = &Chip8::fx_ld_vx_dt;
    this->fxInstructions[0x0015] = &Chip8::fx_ld_dt_vx;
    this->fxInstructions[0x0018] = &Chip8::fx_ld_st_vx;
    this->fxInstructions[0x0029] = &Chip8::fx_ld_f_vx;
    this->fxInstructions[0x0033] = &Chip8::fx_ld_b_vx;
    this->fxInstructions[0x0065] = &Chip8::fx_ld_vx_i;
    this->fxInstructions[0x001E] = &Chip8::fx_add_i_vx;
}

void
Processor::Chip8::debugMemory()
{
  // ensure that fonts are in memory at all times
  for (int i = 0; i < 80; ++i)
  {
    this->memory[i] = chip8_fontset[i];
  }
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

  if (this->delayTimer > 0)
  {
    --this->delayTimer;
  }

  if (this->soundTimer > 0)
  {
    if (this->soundTimer == 1)
    {
      // BEEP!
      std::cout << '\7';
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

  this->registers[MASK(0x0F00) >> 8] = MASK(0x00FF);
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

  unsigned short xCoord = this->registers[MASK(0x0F00) >> 8];
  unsigned short yCoord = this->registers[MASK(0x00F0) >> 4];
  unsigned short height = MASK(0x000F);
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
  if ( this->registers[MASK(0x0F00) >> 8] == MASK(0x00FF) )
  {
    this->programCounter += 4;
  }
  else
  {
    this->programCounter += 2;
  }
}

/*
  4xkk - SNE Vx, byte
    Skip next instruction if Vx != kk.
  
    The interpreter compares register Vx to kk, and if they are not equal,
    increments the program counter by 2.
*/
void
Processor::Chip8::sne_vx_byte()
{
  if ( this->registers[MASK(0x0F00) >> 8] != MASK(0x00FF) )
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

  Fx18 - LD ST, Vx
    Set sound timer = Vx.
  
    ST is set equal to the value of Vx.

*/
void
Processor::Chip8::fx_ld_st_vx()
{
  this->soundTimer = this->registers[MASK(0x0F00) >> 8];
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

  for (int i = 0; i <= (MASK(0x0F00) >> 8); ++i)
  {
    this->registers[i] = this->memory[this->indexRegister + i];
  }
  this->indexRegister += (MASK(0x0F00) >> 8) + 1;
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
  Fx1E - ADD I, Vx
    Set I = I + Vx.

    The values of I and Vx are added, and the results are stored in I.
*/
void
Processor::Chip8::fx_add_i_vx()
{
  std::cout << "fx_add_i_vx: " << hexdump(this->opCode) << std::endl;
  // VF is set to 1 when range overflow (I+VX>0xFFF), and 0
  this->registers[0xF] = 0;
  if ( (this->indexRegister + this->registers[MASK(0x0F00) >> 8]) > 0xFFF )
  {
    this->registers[0xF] = 1;
  }
  this->indexRegister += this->registers[MASK(0x0F00) >> 8];
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

  this->registers[MASK(0x0F00) >> 8] += MASK(0x00FF);
  this->programCounter += 2;
}

/*
  Cxkk - RND Vx, byte
    Set Vx = random byte AND kk.

    The interpreter generates a random number from 0 to 255, which is then ANDed with the value kk. T
    he results are stored in Vx.
*/
void
Processor::Chip8::rnd_vx_byte()
{
  std::cout << "rnd_vx_byte: " << hexdump(this->opCode) << std::endl;

  this->registers[(this->opCode & 0x0F00 ) >> 8] = (rand() % (0xFF + 1)) & (this->opCode & 0x00FF);
  this->programCounter += 2;
}

/*
  Ex9E - SKP Vx
    Skip next instruction if key with the value of Vx is pressed.

    Checks the keyboard, and if the key corresponding to the value of Vx is currently in the down position, 
    PC is increased by 2.


    ExA1 - SKNP Vx
      Skip next instruction if key with the value of Vx is not pressed.

      Checks the keyboard, and if the key corresponding to the value of Vx is currently in the up position, 
      PC is increased by 2.
*/
void
Processor::Chip8::ex_skip()
{
  std::cout << "ex_skip: " << hexdump(this->opCode) << std::endl;

  switch( this->opCode & 0x00FF)
  {

    case 0x009E:
      if ( this->key[( this->registers[MASK(0x0F00) >> 8 ] )] != 0)
      {
        this->programCounter += 4;
      }
      else
      {
        this->programCounter += 2;
      }
      break;
    case 0x00A1:
      if ( this->key[ (this->registers[MASK(0x0F00) >> 8 ]) ] == 0)
      {
        this->programCounter += 4;
      }
      else
      {
        this->programCounter += 2;
      }
      break;
    default:
      std::cout << "Unimplemented EX OpCode: " << hexdump(this->opCode) << std::endl;
      exit(1);

  }
}

/*
  8xy0 - LD Vx, Vy
    Set Vx = Vy.
    
    Stores the value of register Vy in register Vx.
  
  
  8xy1 - OR Vx, Vy
    Set Vx = Vx OR Vy.
    
    Performs a bitwise OR on the values of Vx and Vy, then stores the result in Vx. A bitwise OR compares
    the corrseponding bits from two values, and if either bit is 1, then the same bit in the result 
    is also 1. Otherwise, it is 0. 
  
  
  8xy2 - AND Vx, Vy
    Set Vx = Vx AND Vy.
    
    Performs a bitwise AND on the values of Vx and Vy, then stores the result in Vx. A bitwise 
    AND compares the corrseponding bits from two values, 
    and if both bits are 1, then the same bit in the result is also 1. Otherwise, it is 0. 
  
  
  8xy3 - XOR Vx, Vy
    Set Vx = Vx XOR Vy.
    
    Performs a bitwise exclusive OR on the values of Vx and Vy, then stores the result in Vx. 
    An exclusive OR compares the corrseponding bits from two values, and if the bits are not both the same, 
    then the corresponding bit in the result is set to 1. Otherwise, it is 0. 
  
  
  8xy4 - ADD Vx, Vy
    Set Vx = Vx + Vy, set VF = carry.
    
    The values of Vx and Vy are added together. If the result is greater than 8 bits (i.e., > 255,) 
    VF is set to 1, otherwise 0. Only the lowest 8 bits of the result are kept, and stored in Vx.
  
  
  8xy5 - SUB Vx, Vy
    Set Vx = Vx - Vy, set VF = NOT borrow.
    
    If Vx > Vy, then VF is set to 1, otherwise 0. Then Vy is subtracted from Vx, and the results stored in Vx.
  
  
  8xy6 - SHR Vx {, Vy}
    Set Vx = Vx SHR 1.
    
    If the least-significant bit of Vx is 1, then VF is set to 1, otherwise 0. Then Vx is divided by 2.
  
  
  8xy7 - SUBN Vx, Vy
    Set Vx = Vy - Vx, set VF = NOT borrow.
    
    If Vy > Vx, then VF is set to 1, otherwise 0. Then Vx is subtracted from Vy, and the results stored in Vx.
  
  
  8xyE - SHL Vx {, Vy}
    Set Vx = Vx SHL 1.
    
    If the most-significant bit of Vx is 1, then VF is set to 1, otherwise to 0. Then Vx is multiplied by 2.
*/

void
Processor::Chip8::register_vx_vy_byte()
{
  std::cout << "register_vx_vy_byte: " << hexdump(this->opCode) << std::endl;

  switch (MASK(0x000F))
  {

    case 0x0000:
      this->registers[MASK(0x0F00) >> 8] = this->registers[MASK(0x00F0) >> 4];
      this->programCounter += 2;
      break;

    case 0x0001:
      this->registers[MASK(0x0F00) >> 8] |= this->registers[MASK(0x00F0) >> 4];
      this->programCounter += 2;
      break;

    case 0x0002:
      this->registers[MASK(0x0F00) >> 8] &= this->registers[MASK(0x00F0) >> 4];
      this->programCounter += 2;
      break;

    case 0x0003:
      this->registers[MASK(0x0F00) >> 8] ^= this->registers[MASK(0x00F0) >> 4];
      this->programCounter += 2;
      break;

    case 0x0004:
      this->registers[MASK(0x0F00) >> 8] += this->registers[MASK(0x00F0) >> 4];
      this->registers[0xF] = 0;
      if (this->registers[MASK(0x00F0) >> 4] > (0xFF - this->registers[MASK(0x0F00) >> 8] ))
      {
        // carry flag
        this->registers[0xF] = 1;
      }
      this->programCounter += 2;
      break;

    case 0x0005:
      this->registers[0xF] = 1; // no borrow
      if ( this->registers[MASK(0x00F0) >> 4] > this->registers[MASK(0x0F00) >> 8] )
      {
        this->registers[0xF] = 0; // there is a borrow
      }
      this->registers[MASK(0x0F00) >> 8] -= this->registers[MASK(0x00F0) >> 4];
      this->programCounter += 2;
      break;

    case 0x0006:
      this->registers[0xF] = this->registers[MASK(0x0F00) >> 8] & 0x1;
      this->registers[MASK(0x0F00) >> 8] >>= 1;
      this->programCounter += 2;
      break;

    case 0x0007:
      this->registers[0xF] = 1; // no borrow
      if ( this->registers[MASK(0x0F00) >> 8] > this->registers[MASK(0x00F0) >> 4] )
      {
        this->registers[0xF] = 0; // borrow
      }
      this->registers[MASK(0x0F00) >> 8] = this->registers[MASK(0x00F0) >> 4] - this->registers[MASK(0x0F00) >> 8];
      this->programCounter += 2;
      break;

    case 0x000E:
      this->registers[0xF] = this->registers[MASK(0x0F00) >> 8] >> 7;
      this->registers[MASK(0x0F00) >> 8] <<= 1;
      this->programCounter += 2;
      break;

    default:
      std::cout << "Unimplemented register_vx_vy_byte OpCode: " << hexdump(this->opCode) << std::endl;
      exit(1);
  }

}





