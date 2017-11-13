#ifndef __Processor
#define __Processor 1

#include <stdio.h>
#include <stdlib.h>
#include <random>
#include "time.h"
#include <map>
#include <iostream>
#include <cstdint>
#include <string>
#include <fstream>
#include "debug/hexdump.hpp"

#define C8_MEMORY_OFFSET     512
#define C8_MEMORY_OFFSET_HEX 0x200
#define C8_EMULATION_SPEED_SLEEP 1200
#define C8_GFX_LENGTH 64
#define C8_GFX_WIDTH  32

#define MASK(hex) ((this->opCode & hex))

namespace Processor
{
	class Chip8
	{

    typedef void(Chip8::*instructionHandle)();

    private:
      uint16_t stack[16];         // Stack
      uint16_t sp;                // Stack pointer
      uint8_t  memory[4096];      // 4k of memory
      uint8_t  registers[16];     // 16 registers: 0 - F
      uint16_t indexRegister;
      uint16_t programCounter;
      uint16_t opCode;

      uint8_t delayTimer;                // Delay timer
      uint8_t soundTimer;                // Sound timer

      std::string filename;
      std::ifstream file;
      std::map<uint16_t, instructionHandle> instructions;
      std::map<uint16_t, instructionHandle> fxInstructions;   // there's a lot of 0xFxNN instructions

    public:

      bool    drawFlag; // tell the view to redraw the screen
      uint8_t graphicsBuffer[C8_GFX_LENGTH * C8_GFX_WIDTH];
      uint8_t  key[16]; // Keypad

      Chip8(const char *file_path);
      void initialize();
      void dumpMemory();
      void cycle();

    protected:
      void return_clear_screen();
      void fx_entrance();
      void ex_skip();
      void register_vx_vy_byte();
      void fx_ld_b_vx();
      void fx_ld_vx_i();
      void fx_ld_f_vx();
      void fx_ld_dt_vx();
      void fx_ld_vx_dt();
      void fx_ld_st_vx();
      void fx_add_i_vx();
      void ld_vx_byte();
      void ld_i_addr();
      void drw_vx_vy_nibble();
      void call_addr();
      void add_vx_byte();
      void se_vx_byte();
      void sne_vx_byte();
      void jp_addr();
      void rnd_vx_byte();

	};
}

#endif