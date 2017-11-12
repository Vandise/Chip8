#ifndef __Processor
#define __Processor 1

#include <iostream>
#include <cstdint>
#include <string>
#include <fstream>
#include "debug/hexdump.hpp"

#define C8_MEMORY_OFFSET 512

namespace Processor
{
	class Chip8
	{

    private:
      uint8_t memory[4096];

      std::string filename;
      std::ifstream file;

    public:
      Chip8(const char *file_path);
      void initialize();
      void dumpMemory();

	};
}

#endif