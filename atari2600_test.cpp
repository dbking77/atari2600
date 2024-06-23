#include <gtest/gtest.h>

#include "atari2600.hpp"
#include "tia.hpp"
#include "util.hpp"

#include <fstream>
#include <iomanip>

TEST(reverseBits32, simple)
{
  EXPECT_EQ(reverseBits32(0x00000001), 0x80000000) << std::hex << reverseBits32(0x00000001);
  EXPECT_EQ(reverseBits32(0x00000002), 0x40000000);
  EXPECT_EQ(reverseBits32(0x00000004), 0x20000000);
  EXPECT_EQ(reverseBits32(0x00000008), 0x10000000);
  EXPECT_EQ(reverseBits32(0x00000007), 0xE0000000);
  EXPECT_EQ(reverseBits32(0x10000007), 0xE0000008);


  EXPECT_EQ(reverseBits32(0x15C62907), 0xE09463A8);

}

TEST(reverseBits32, coverage)
{
  uint32_t values[] = {
    0x36917300,
    0xAFBC0000,
    0x11111111,
    0x12345678,
    0xABCDEF23
  };

  for (uint32_t value : values)
  {
    EXPECT_EQ(reverseBits32(value), reverseBits32Slow(value));
    EXPECT_EQ(reverseBits32(reverseBits32(value)), value);
  }
}

TEST(reverseBits8, simple)
{
  EXPECT_EQ(reverseBits8(0x01), 0x80) << std::hex << reverseBits32(0x00000001);
  EXPECT_EQ(reverseBits8(0x02), 0x40);
  EXPECT_EQ(reverseBits8(0x04), 0x20);
  EXPECT_EQ(reverseBits8(0x08), 0x10);
  EXPECT_EQ(reverseBits8(0x07), 0xE0);
  EXPECT_EQ(reverseBits8(0x17), 0xE8);
  EXPECT_EQ(reverseBits8(0x15), 0xA8);
}

TEST(reverseBits8, coverage)
{
  for (unsigned value = 0; value <= 0xFF; ++value)
  {
    EXPECT_EQ(reverseBits8(value), reverseBits8Slow(value));
    EXPECT_EQ(reverseBits8(reverseBits8(value)), value);
  }
}


/**
 * Test that running instructions 1 at a time produces same results as running instructions in large chunk.
*/
TEST(Atari2600, execMultiple)
{
  std::vector<Atari2600> ataris(2);
  auto& atari0 = ataris.at(0);
  auto& atari1 = ataris.at(1);

  std::string rom_fn = "playfield_colors_out.bin";
  std::string palette_fn = "palette/REALNTSC.pal";

  // Load ROM and palette
  for (auto& atari : ataris)
  {
    std::ifstream rom_input(rom_fn, std::ifstream::binary);
    ASSERT_TRUE(rom_input.good());
    atari.loadRom(rom_input);
    std::ifstream palette_input(palette_fn, std::ifstream::binary);
    ASSERT_TRUE(palette_input.good());
    atari.tia_.loadPalette(palette_input);
  }

  unsigned instruction_count = 1000;
  for (unsigned ii = 0; ii < instruction_count; ++ii)
  {
    atari0.execInstructions(1);
  }

  atari1.execInstructions(instruction_count);

  EXPECT_EQ(atari0.tia_.scan_x_, atari1.tia_.scan_x_);
  EXPECT_EQ(atari0.tia_.scan_y_, atari1.tia_.scan_y_);

  EXPECT_EQ(atari0.cpu_.instr_cycle_count_, atari1.cpu_.instr_cycle_count_);
  EXPECT_EQ(atari0.tia_.pixel_count_, atari1.tia_.pixel_count_);

  // Display's should match
  for (unsigned y = 0; y < Tia::DISPLAY_HEIGHT; ++y)
  {
    for (unsigned x = 0; x < Tia::DISPLAY_WIDTH; ++x)
    {
      ASSERT_EQ(atari0.tia_.getDisplay(x, y), atari1.tia_.getDisplay(x, y));
    }
  }
}


TEST(Tia, usePlayer)
{
  for (int offset_x = 0; offset_x < 192; offset_x += 1)
  {
    // For all 8bit set, the play should be displayed from offset+0 to offset+7
    EXPECT_TRUE(Tia::usePlayer(0xFF, offset_x + 0, offset_x + 0));
    EXPECT_TRUE(Tia::usePlayer(0xFF, offset_x + 0, offset_x + 7));
    EXPECT_FALSE(Tia::usePlayer(0xFF, offset_x + 0, offset_x - 1));
    EXPECT_FALSE(Tia::usePlayer(0xFF, offset_x + 0, offset_x + 8));
    EXPECT_FALSE(Tia::usePlayer(0xFF, offset_x + 0, offset_x - 10));
    EXPECT_FALSE(Tia::usePlayer(0xFF, offset_x + 0, offset_x + 10));
  }

  for (int display_x = 0; display_x < 192; display_x += 1)
  {
    for (uint8_t position_x = 0; position_x < (192-8); ++position_x)
    {
      ASSERT_EQ(Tia::usePlayer(0xA7, position_x, display_x), Tia::usePlayerSlow(0xA7, position_x, display_x))
        << std::dec
        << " position " << static_cast<int>(position_x)
        << " display " << display_x;
    }
  }

  //uint8_t mask = 0xFF;
}