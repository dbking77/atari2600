#ifndef ATARI2600_TIA_HPP_GUARD
#define ATARI2600_TIA_HPP_GUARD

#include <array>
#include <cstdint>
#include <iostream>
#include <optional>
#include <vector>

// atari doesn't really have a display buffer
// but need to store scanline data somewhere
struct RGBA
{
  union {
    struct {
        uint8_t r;
        uint8_t g;
        uint8_t b;
        uint8_t a;
    };
    uint32_t raw32;
  };
};

inline bool operator==(RGBA a, RGBA b)
{
  return a.raw32 == b.raw32;
}

struct TiaSettings
{
  uint32_t pf_mask = 0;
  uint8_t ctrl_pf = 0;
  uint8_t p0_mask = 0;
  uint8_t p1_mask = 0;
  uint8_t color_pf = 0;
  uint8_t color_bk = 0;
  uint8_t color_p0 = 0;
  uint8_t color_p1 = 0;
  bool reflect_p0 = false;
  bool reflect_p1 = false;
  RGBA rgba_pf = {0,0,0,0};
  RGBA rgba_bk = {0,0,0,0};
  RGBA rgba_p0 = {0,0,0,0};
  RGBA rgba_p1 = {0,0,0,0};
};

/**
 * Television interface adapter
 */
class Tia
{
public:
  Tia();

  TiaSettings settings_;
  TiaSettings next_settings_;
  bool settings_changed_ = false;

  enum
  {
    VSYNC_ADDR = 0x0,
    VBLANK_ADDR = 0x1,
    WSYNC_ADDR = 0x2,
    RSYNC_ADDR = 0x3,
    NUSIZ0_ADDR = 0x4,
    NUSIZ1_ADDR = 0x5,
    COLUP0_ADDR = 0x6,
    COLUP1_ADDR = 0x7,
    COLUPF_ADDR = 0x8,
    COLUBK_ADDR = 0x9,
    CTRLPF_ADDR = 0xA,
    REFP0_ADDR = 0xB,
    REFP1_ADDR = 0xC,
    PF0_ADDR = 0xD,
    PF1_ADDR = 0xE,
    PF2_ADDR = 0xF,
    RESP0_ADDR = 0x10,
    RESP1_ADDR = 0x11,
    RESM0_ADDR = 0x12,
    RESM1_ADDR = 0x13,
    RESBL_ADDR = 0x14,
    AUDC0_ADDR = 0x15,
    AUDC1_ADDR = 0x16,
    AUDF0_ADDR = 0x17,
    AUDV0_ADDR = 0x18,
    AUDV1_ADDR = 0x19,
    AUDF1_ADDR = 0x1A,
    GRP0_ADDR = 0x1B,
    GRP1_ADDR = 0x1C,
    ENAM0_ADDR = 0x1D,
    ENAM1_ADDR = 0x1E,
    ENABL_ADDR = 0x1F,
    HMPL0_ADDR = 0x20,
    HMPL1_ADDR = 0x21,
    HMPM0_ADDR = 0x22,
    HMPM1_ADDR = 0x23,
    HMPBL_ADDR = 0x24,
    VDELP0_ADDR = 0x25,
    VDELP1_ADDR = 0x26,
    VDELBL_ADDR = 0x27,
    RESMP0_ADDR = 0x28,
    RESMP1_ADDR = 0x29,
    HMOVE_ADDR = 0x2A,
    HMCLR_ADDR = 0x2B,
    CXCLR_ADDR = 0x2C
  };

  static const char* addrName(uint16_t);

  // Color palette (NTSC / PAL)
  std::array<RGBA, 256> palette_;

  // set if tia should do a wsync with next update
  bool wait_sync_ = false;

  // set if tia is performing a vertical sync
  bool vertical_sync_ = false;

  bool reset_p0_ = false;
  bool reset_p1_ = false;

  // display locations for player 0 and 1, 0xFF means image will not be displayed
  uint8_t position_x_p0_ = 0xFF;
  uint8_t position_x_p1_ = 0xFF;

  static inline bool usePlayer(uint8_t mask, uint8_t position_x, int display_x)
  {
    int offset = (display_x - position_x);
    return (offset & ~7) ? 0 : (mask >> offset) & 1;
  }

  static bool usePlayerSlow(uint8_t mask, uint8_t position_x, int display_x);

  void loadPalette(std::istream& input);

  static constexpr int DISPLAY_WIDTH = 160;
  static constexpr int HORIZONTAL_BLANK = 68;

  static constexpr int DISPLAY_NOMINAL_HEIGHT = 192;
  static constexpr int VERTICAL_SYNC = 3;
  static constexpr int VERTICAL_BLANK = 37;
  static constexpr int OVERSCAN = 30;
  static constexpr int DISPLAY_HEIGHT = DISPLAY_NOMINAL_HEIGHT + VERTICAL_BLANK + OVERSCAN;
  static constexpr int AUTO_VSYNC = DISPLAY_HEIGHT + 100;


  std::vector<RGBA> display_;

  /**
   * @brief Advance a certain amount of pixels, should be called after each CPU instruction completes
   * Function is lazy and qill drawing to display buffer unless some previous
   * insruction changes a display setting
   */
  void advancePixels(unsigned pixel_cycles);

  unsigned pixel_cycles_ = 0;
  unsigned pixel_count_ = 0;

  /**
   * Sync(hronize) any undrawn pixels to display buffer
   */
  void syncPixels();

  void clearDisplay();

  static inline int scanToDisplayX(int scan_x) {return scan_x - HORIZONTAL_BLANK;}
  static inline int scanToDisplayY(int scan_y) {return scan_y - VERTICAL_BLANK;}

  uint8_t getPlayerPositionX() const;

  int scan_x_ = -1; // HORIZONTAL_BLANK;
  int scan_y_ = 0; // VERTICAL_BLANK

  RGBA& getDisplay(unsigned display_x, unsigned scan_y)
  {
    return display_.at(scan_y*DISPLAY_WIDTH + display_x);
  }

  /**
   * Updates pixel line with number of pixels cycles
   * Returns number of pixel cycles remaining (if end of pixel line is reached)
   */
  unsigned drawPixelLine(unsigned pixel_cycles);

  void drawPixels(unsigned pixel_cycles);

  uint8_t read(uint16_t addr);
  void write(uint16_t addr, uint8_t data);
};

#endif  // ATARI2600_TIA_HPP_GUARD