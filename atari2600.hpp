#ifndef ATARI2600_ATARI2600_HPP_GUARD
#define ATARI2600_ATARI2600_HPP_GUARD

#include <array>
#include <cstdint>
#include <iostream>
#include <unordered_set>
#include <vector>

#include "mos6502.hpp"
#include "tia.hpp"

class Atari2600
{
public:
  Atari2600();

  Mos6502 cpu_;
  Tia tia_;
  std::array<uint8_t, 128> ram_;
  std::vector<uint8_t> rom_;

  void loadRom(std::istream& in);

  static constexpr unsigned ROM_SIZE = 1<<12; // 4k ROM

  void execInstructions(unsigned instruction_count);
  void addBreakpoint(uint16_t addr);
  void clearBreakpoints();

protected:
  std::unordered_set<uint16_t> breakpoints_;

  uint8_t read(uint16_t addr);
  void write(uint16_t addr, uint8_t data);
};

#endif  // ATARI2600_ATARI2600_HPP_GUARD