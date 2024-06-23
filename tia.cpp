#include "tia.hpp"
#include "util.hpp"

#include <algorithm>
#include <cassert>

Tia::Tia()
{
  display_.resize(DISPLAY_WIDTH * DISPLAY_HEIGHT, RGBA{0,0,0,0});
  clearDisplay();
  std::fill(palette_.begin(), palette_.end(), RGBA{0,0,0,0});
}


void Tia::clearDisplay()
{
  std::cerr << "clear display" << std::endl;
  for (unsigned y = 0; y < DISPLAY_HEIGHT; ++y)
  {
    for (unsigned x = 0; x < DISPLAY_WIDTH; ++x)
    {
      getDisplay(x, y) = ((x ^ y) & 1) ? RGBA{0,0,0,255} : RGBA{255,255,255,255};
    }
  }
}

// https://forums.atariage.com/topic/204247-new-generated-ntsc-color-palette-files/#comment-2621055
// https://www.randomterrain.com/atari-2600-memories-tutorial-andrew-davie-11.html
void Tia::loadPalette(std::istream& input)
{
  constexpr unsigned PALETTE_SIZE = 3*256; // 3 bytes per color (RGB) * 256 settings
  std::array<uint8_t, PALETTE_SIZE> palette_rgb;
  input.read(reinterpret_cast<char*>(palette_rgb.data()), PALETTE_SIZE);
  if (!input)
  {
    std::cerr << "ERROR, only read " << input.gcount() << " bytes from palette file, expected " << PALETTE_SIZE << std::endl;
  }

  // Copy RGB to RGBA
  for (unsigned ii = 0; ii < 256; ++ii)
  {
    palette_.at(ii) = RGBA{
      palette_rgb.at(ii*3+0),
      palette_rgb.at(ii*3+1),
      palette_rgb.at(ii*3+2),
      255
      };
  }
}

unsigned Tia::drawPixelLine(unsigned pixel_cycles)
{
  if (pixel_cycles == 0)
  {
    return 0;
  }

  if (vertical_sync_)
  {
    if ((scan_y_ != 0) or (scan_x_ != -1))
    {
      clearDisplay();
    }

    scan_x_ = -1;
    scan_y_ = 0;
    pixel_count_ += pixel_cycles;
    return 0;
  }

  //static constexpr unsigned DISPLAY_START = HORIZONTAL_BLANK;
  //static constexpr unsigned DISPLAY_END = HORIZONTAL_BLANK + DISPLAY_WIDTH;
  if (scan_x_ >= (HORIZONTAL_BLANK + DISPLAY_WIDTH - 1))
  {
    // start next scan line
    scan_x_ = -1;

    ++scan_y_;

    // automatically start next screen if VSYNC doesn't occur after a while
    if (scan_y_ >= AUTO_VSYNC)
    {
      std::cerr << "forcing screen refreshed (needed veritical sync)" << std::endl;
      scan_y_ = 0;
      clearDisplay();
    }
  }

  if (scan_x_ < (HORIZONTAL_BLANK - 1))
  {
    unsigned pixels_to_line_start = (HORIZONTAL_BLANK - 1) - scan_x_;
    //std::cerr << " pixel_cycles " << std::dec << pixel_cycles << " pixels to line start " << pixels_to_line_start << std::endl;
    if (pixel_cycles <= pixels_to_line_start)
    {
      scan_x_ += pixel_cycles;
      pixel_count_ += pixel_cycles;
      return 0;
    }
    else
    {
      pixel_cycles -= pixels_to_line_start;
      scan_x_ = HORIZONTAL_BLANK - 1;
    }
  }

  // Don't draw anything beyond display limits
  if (scan_y_ >= DISPLAY_HEIGHT)
  {
      std::cerr << "overdraw " << scan_y_ << std::endl;
      return 0;
  }

  assert(scan_x_ >= (HORIZONTAL_BLANK - 1));

  unsigned pixels_to_line_end = (HORIZONTAL_BLANK + DISPLAY_WIDTH - 1) - scan_x_;
  unsigned display_cycles = std::min(pixel_cycles, pixels_to_line_end);
  int display_x = scanToDisplayX(scan_x_ + 1);
  //std::cerr << " scan_x " << std::dec << scan_x_ << " dis play_x " << display_x << std::endl;
  assert(display_x >= 0);
  assert(display_x < DISPLAY_WIDTH);
  int display_x_stop = display_x + display_cycles;
  scan_x_ += display_cycles;
  pixel_cycles -= display_cycles;

  uint64_t pf = settings_.pf_mask;
  bool reflect = settings_.ctrl_pf & 1;
  if (reflect)
  {
    pf |= static_cast<uint64_t>(reverseBits32(pf << 12)) << 20;
  }
  else
  {
    pf |= (pf & 0xFFFFF) << 20;
  }

  uint8_t p0_mask = settings_.reflect_p0 ? reverseBits8(settings_.p0_mask) : settings_.p0_mask;
  uint8_t p1_mask = settings_.reflect_p1 ? reverseBits8(settings_.p1_mask) : settings_.p1_mask;

  assert(scan_y_ >= 0);
  assert(scan_y_ < DISPLAY_HEIGHT);

  for  (; display_x < display_x_stop; ++display_x)
  {
    unsigned pf_idx = (display_x >> 2);
    bool use_pf = (pf >> pf_idx) & 1;
    bool use_p0 = usePlayer(p0_mask, position_x_p0_, display_x);
    bool use_p1 = usePlayer(p1_mask, position_x_p1_, display_x);
    RGBA rgba =
      use_p0 ? settings_.rgba_p0 :
      use_p1 ? settings_.rgba_p1 :
      use_pf ? settings_.rgba_pf :
      settings_.rgba_bk;
    getDisplay(display_x, scan_y_) = rgba;
  }

  pixel_count_ += display_cycles;
  return pixel_cycles;
}


void Tia::syncPixels()
{
  //std::cerr << std::dec << pixel_count_ << " syncPixels " << std::dec << pixel_cycles_ << std::endl;
  while (pixel_cycles_ > 0)
  {
    pixel_cycles_ = drawPixelLine(pixel_cycles_);
  }
}


void Tia::advancePixels(unsigned pixel_cycles)
{
  pixel_cycles_ += pixel_cycles;

  if (settings_changed_)
  {
    //std::cerr << std::dec << pixel_count_  << " settings changed (before sync)" << std::endl;
    settings_changed_ = false;
    syncPixels();
    std::cerr << std::dec << pixel_count_  << " settings changed (after sync)" << std::endl;
    settings_ = next_settings_;
  }

  if (wait_sync_)
  {
    wait_sync_ = false;
    pixel_cycles_ = 0;
    int pixel_cycles_to_line_end = (HORIZONTAL_BLANK + DISPLAY_WIDTH - 1) - scan_x_;
    assert(pixel_cycles_to_line_end >= 0);
    //std::cout << std::dec << pixel_count_ << " before wsync on scan_y " << std::dec << scan_y_ << " remaining pixels " << pixel_cycles_to_line_end << std::endl;
    unsigned remaining_cycles = drawPixelLine(pixel_cycles_to_line_end);
    std::cerr << std::dec << pixel_count_  << " after wsync" << std::endl;
    assert(remaining_cycles == 0);
  }

  if (reset_p0_)
  {
    reset_p0_ = false;
    position_x_p0_ = getPlayerPositionX();
  }

  if (reset_p1_)
  {
    reset_p1_ = false;
    position_x_p1_ = getPlayerPositionX();
  }
}

uint8_t Tia::getPlayerPositionX() const
{
  int display_x = scanToDisplayX(scan_x_);
  return std::clamp(display_x, 0, 255);
}

uint8_t Tia::read(uint16_t addr)
{
    return 0;
}


void Tia::write(uint16_t addr, uint8_t data)
{
  // TIA only has 6 address pins
  addr &= 0x3F;

  std::cerr << "TIA write " <<  std::hex << static_cast<int>(data) << " to ADDR_" << std::hex << addr << " " << addrName(addr) << std::dec << std::endl;
  // Assume a write will probably change a drawing settings
  bool settings_changed = true;
  switch (addr)
  {
    case WSYNC_ADDR:
      wait_sync_ = true;
      break;
    case VSYNC_ADDR:
      vertical_sync_ = data & 2;
      //std::cerr << " vertical sync change to " << vertical_sync_ << std::endl;
      break;
    case COLUP0_ADDR:
      next_settings_.color_p0 = data;
      next_settings_.rgba_p0 = palette_.at(data);
      break;
    case COLUP1_ADDR:
      next_settings_.color_p1 = data;
      next_settings_.rgba_p1 = palette_.at(data);
      break;
    case COLUPF_ADDR:
      next_settings_.color_pf = data;
      next_settings_.rgba_pf = palette_.at(data);
      break;
    case COLUBK_ADDR:
      next_settings_.color_bk = data;
      next_settings_.rgba_bk = palette_.at(data);
      break;
    case CTRLPF_ADDR:
      next_settings_.ctrl_pf = data;
      break;
    case REFP0_ADDR:
      next_settings_.reflect_p0 = data & (1<<3);
      break;
    case REFP1_ADDR:
      next_settings_.reflect_p1 = data & (1<<3);
      break;
    case PF0_ADDR:
      next_settings_.pf_mask &= ~0xF;
      next_settings_.pf_mask |= (data >> 4) & 0xF;
      break;
    case PF1_ADDR:
      next_settings_.pf_mask &= ~0xFF0;
      // for whatever reason PF1 bits get draw MSB first instead of LSB first
      next_settings_.pf_mask |= reverseBits8(data) << 4;
      break;
    case PF2_ADDR:
      next_settings_.pf_mask &= ~0xFF000;
      next_settings_.pf_mask |= data << 12;
      break;
    case RESP0_ADDR:
      reset_p0_ = true;
      break;
    case RESP1_ADDR:
      reset_p1_ = true;
      break;
    case GRP0_ADDR:
      next_settings_.p0_mask = data;
      break;
    case GRP1_ADDR:
      next_settings_.p1_mask = data;
      break;
    default:
      settings_changed = false;
      break;
  }

  settings_changed_ = settings_changed;
}


const char* Tia::addrName(uint16_t addr)
{
  switch (addr)
  {
  case VSYNC_ADDR: return "VSYNC";
  case VBLANK_ADDR: return "VBLANK";
  case WSYNC_ADDR: return "WSYNC";
  case RSYNC_ADDR: return "RSYNC";
  case NUSIZ0_ADDR: return "NUSIZ0";
  case NUSIZ1_ADDR: return "NUSIZ1";
  case COLUP0_ADDR: return "COLUP0";
  case COLUP1_ADDR: return "COLUP1";
  case COLUPF_ADDR: return "COLUPF";
  case COLUBK_ADDR: return "COLUBK";
  case CTRLPF_ADDR: return "CTRLPF";
  case REFP0_ADDR: return "REFP0";
  case REFP1_ADDR: return "REFP1";
  case PF0_ADDR: return "PF0";
  case PF1_ADDR: return "PF1";
  case PF2_ADDR: return "PF2";
  case RESP0_ADDR: return "RESP0";
  case RESP1_ADDR: return "RESP1";
  case RESM0_ADDR: return "RESM0";
  case RESM1_ADDR: return "RESM1";
  case RESBL_ADDR: return "RESBL";
  case GRP0_ADDR: return "GRP0";
  case GRP1_ADDR: return "GRP1";
  case ENAM0_ADDR: return "ENAM0";
  case ENAM1_ADDR: return "ENAM1";
  case ENABL_ADDR: return "ENABL";
  case HMPL0_ADDR: return "HMPL0";
  case HMPL1_ADDR: return "HMPL1";
  case HMPM0_ADDR: return "HMPM0";
  case HMPM1_ADDR: return "HMPM1";
  case HMPBL_ADDR: return "HMPBL";
  case VDELP0_ADDR: return "VDELP0";
  case VDELP1_ADDR: return "VDELP1";
  case VDELBL_ADDR: return "RESMBL";
  case RESMP0_ADDR: return "RESMP0";
  case RESMP1_ADDR: return "RESMP1";
  case HMOVE_ADDR: return "HMOVE";
  case HMCLR_ADDR: return "HMCLR";
  case CXCLR_ADDR: return "CXCLR";
  default: return "?";
  }
}


bool Tia::usePlayerSlow(uint8_t mask, uint8_t position_x, int display_x)
{
  if (position_x == 0xFF)
  {
    return false;
  }
  int offset = display_x - position_x;
  if ((offset >= 8) or (offset < 0))
  {
    return false;
  }
  return mask & (1 << offset);
}
