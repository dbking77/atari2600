#include "atari2600.hpp"

#include <iomanip>

Atari2600::Atari2600() :
  cpu_{
    [this](uint16_t addr) -> uint8_t {
      return this->read(addr);
    },
    [this](uint16_t addr, uint8_t data) {
      this->write(addr, data);
    }
    }
{
  rom_.resize(ROM_SIZE, 0);
  std::fill(ram_.begin(), ram_.end(), 0);
}

void Atari2600::loadRom(std::istream& in)
{
  rom_.clear();
  rom_.resize(ROM_SIZE, 0);
  in.read(reinterpret_cast<char*>(rom_.data()), ROM_SIZE);
  if (!in)
  {
    std::cerr << "WARNING, only read " << in.gcount() << " bytes from file to ROM" << std::endl;
  }
}

// https://forums.atariage.com/topic/192418-mirrored-memory/#comment-2439795
uint8_t Atari2600::read(uint16_t addr)
{
  // address is 13-bit at most because 6507 has only 13 address pins
  addr &= 0x1FFF;

  if (addr & 0x1000)
  {
    // Addresses 0x1000 to 0x1FFF are rom
    //std::cerr << "Read rom " << (addr & 0xFFF) << std::endl;
    return rom_[addr & 0xFFF];
  }
  else if (addr & 0x80)
  {
    // 6532 RIOT CHIP : Chipselect A12 = 0, and A7 = 1
    // PIA 532
    if (addr & 0x200)
    {
      // TODO timer registers
      switch (addr)
      {
        case 0x280:  // SWCHA
          // https://alienbill.com/2600/101/docs/stella.html#pia5.0
          std::cerr << "SWCHA read" << std::endl;
          // 0 = pressed, 1 not pressed
          // Bit7 : P0 right
          // Bit6 : P0 left
          // Bit5 : P0 down
          // Bit4 : P0 up
          // Bit3 : P1 right
          // Bit2 : P1 left
          // Bit1 : P1 down
          // Bit0 : P1 up
          return 0xFF;  // nothing pressed (for now)

        case 0x282:  // SWCHB
          // https://alienbill.com/2600/101/docs/stella.html#pia4.0
          std::cerr << "SWCHB read" << std::endl;
          // Bit7 : P1 difficulty 0= Amature
          // Bit6 : P0 difficulty 1= Pro
          // Bit5-4 : unused
          // Bit3 : Color =1, B/W =0
          // Bit2 : unused
          // Bit1 : Game select 0=pressed
          // Bit0 : Game reset 0=pressed
          return 0x7F;
      }
    }
    else
    {
      return ram_[addr & 0x7F];
    }
    // TODO
  }

  return 0;
}



void Atari2600::write(uint16_t addr, uint8_t data)
{
  // address is 13-bit at most because 6507 has only 13 address pins
  addr &= 0x1FFF;

  if (false)
  {
  std::cerr << "Write "
            << std::hex << std::setw(2) << std::setfill('0') << static_cast<unsigned>(data)
            << " to "
            << std::hex << std::setw(4) << std::setfill('0') << addr
            << std::endl;
  }

  if ((addr & (1<<12)) == 0)
  {
    // A12 = 0
    if (addr & (1<<7))
    {
      // 6532 RIOT CHIP : Chipselect A12 = 0, and A7 = 1
      if (addr & 0x200)
      {
        // TODO 6532 timer registers
      }
      else
      {
        // 6532 128bytes of RAM
        ram_[addr & 0x7F] = data;
      }
    }
    else
    {
      // TIA chip : Chipselect A12 = 0 and A7 = 0
      tia_.write(addr, data);
    }
  }
}

void Atari2600::execInstructions(unsigned instruction_count)
{
  for (unsigned ii = 0; ii < instruction_count; ++ii)
  {
    unsigned pixel_cycles = cpu_.execOne() * 3;
    tia_.advancePixels(pixel_cycles);
    if (breakpoints_.count(cpu_.pc_))
    {
      std::cerr << "Hit breakpoint at " << std::hex << cpu_.pc_ << std::dec << std::endl;
      break;
    }
  }
  // Force a "sync" of tia
  tia_.syncPixels();
}

void Atari2600::addBreakpoint(uint16_t addr)
{
  breakpoints_.insert(addr);
}

void Atari2600::clearBreakpoints()
{
    breakpoints_.clear();
}
