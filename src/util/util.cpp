#include "util.hpp"

namespace util
{
    uint32_t popcount(uint32_t i)
    {
        // https://stackoverflow.com/questions/109023/how-to-count-the-number-of-set-bits-in-a-32-bit-integer
        // TODO : move and come back later replace with intrinsics or fall back to this
        i = i - ((i >> 1) & 0x55555555);
        i = (i & 0x33333333) + ((i >> 2) & 0x33333333);
        return (((i + (i >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
    }
}