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
#include <chrono>
#include <thread>
#include "debug/hexdump.hpp"

#define C8_MEMORY_OFFSET     512
#define C8_MEMORY_OFFSET_HEX 0x200
#define C8_EMULATION_SPEED_SLEEP 1200
#define C8_GFX_LENGTH 64
#define C8_GFX_WIDTH  32

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

      std::string filename;
      std::ifstream file;
      std::map<uint16_t, instructionHandle> instructions;

    public:

      bool    drawFlag;          // tell the view to redraw the screen
      uint8_t graphicsBuffer[C8_GFX_LENGTH * C8_GFX_WIDTH];

      Chip8(const char *file_path);
      void initialize();
      void dumpMemory();
      void cycle();

    protected:
      void ld_vx_byte();
      void ld_i_addr();
      void drw_vx_vy_nibble();
      void call_addr();

	};
}

#endif