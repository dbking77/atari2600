#include "util.hpp"

uint32_t reverseBits32Slow(uint32_t value)
{
    uint32_t result;
    for (unsigned ii = 0; ii < 32; ++ii)
    {
        result = (result << 1) | ((value >> ii) & 1);
    }
    return result;
}

uint8_t reverseBits8Slow(uint8_t value)
{
    uint8_t result;
    for (unsigned ii = 0; ii < 8; ++ii)
    {
        result = (result << 1) | ((value >> ii) & 1);
    }
    return result;
}
