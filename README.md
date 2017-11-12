Chip-8 CPU Emulator
---
Chip8 Emulator

Dependencies:

- libsdl2-dev

## How to use this project
You may have to change some flags and/or compiler on your system before running the make command. By default it utilizes gcc.

To compile, run

```bash
$ make
```

This will create an executable called `c8` in your `bin` directory.

### c8
c8 takes one parameter, the file you want to run. There's only one file currently, `pong` in the resources directory. To execute this program, run the following in your terminal

```bash
$ ./bin/c8 resources/pong
```
