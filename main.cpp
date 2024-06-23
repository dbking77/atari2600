#include <iostream>
#include "mos6502.hpp"

uint8_t instructions[] =
{
  0xA2, 0x12,  // LDX #12
  0xA9, 0xFF,  // LDA #FF
  0xA0, 0x34,  // LDY #34
  0xEA, // NOP
  0xEA, // NOP
};

uint8_t read_callback(uint16_t addr)
{
  return instructions[addr & 0x7];
}

void write_callback(uint16_t addr, uint8_t data)
{
}

int main()
{
  Mos6502 proc{read_callback, write_callback};
  for (unsigned cycle = 0; cycle < 10; )
  {
    std::cout << "Cycle " << std::dec << cycle << std::endl;
    cycle += proc.execOne();
    proc.outputRegs(std::cout);
  }
  return 0;
}