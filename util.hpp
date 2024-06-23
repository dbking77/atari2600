#ifndef ATARI2600_UTIL_HPP_GUARD
#define ATARI2600_UTIL_HPP_GUARD

#include <cstdint>

inline uint32_t reverseBits32(uint32_t value)
{
    value = (value >> 16) | (value << 16);
    value = ((value & 0xFF00FF00) >> 8) | ((value & 0x00FF00FF) << 8);
    value = ((value & 0xF0F0F0F0) >> 4) | ((value & 0x0F0F0F0F) << 4);
    value = ((value & 0xCCCCCCCC) >> 2) | ((value & 0x33333333) << 2);
    value = ((value & 0xAAAAAAAA) >> 1) | ((value & 0x55555555) << 1);
    return value;
}

inline uint8_t reverseBits8(uint8_t value)
{
    value = ((value & 0xF0) >> 4) | ((value & 0x0F) << 4);
    value = ((value & 0xCC) >> 2) | ((value & 0x33) << 2);
    value = ((value & 0xAA) >> 1) | ((value & 0x55) << 1);
    return value;
}

uint32_t reverseBits32Slow(uint32_t value);
uint8_t reverseBits8Slow(uint8_t value);

#endif  // ATARI2600_UTIL_HPP_GUARD