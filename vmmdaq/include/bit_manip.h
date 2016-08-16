#include "stdio.h"

namespace bits {

    struct endian_swap32 : public std::unary_function<uint32_t, uint32_t>
    {
        endian_swap32(){};
        uint32_t operator() (const uint32_t& x_) {
            uint32_t x = x_;
            return
            ((x>>24) & 0x000000FF) |
            ((x<<8)  & 0x00FF0000) |
            ((x>>8)  & 0x0000FF00) |
            ((x<<24) & 0xFF000000); 
        }
    };

    struct reverse_32 : public std::unary_function<uint32_t, uint32_t>
    {
        reverse_32(){};
        uint32_t operator() (const uint32_t& x_) {
            uint32_t x = x_;
            x = (((x & 0xaaaaaaaa) >> 1) | ((x & 0x55555555) << 1));
            x = (((x & 0xcccccccc) >> 2) | ((x & 0x33333333) << 2));
            x = (((x & 0xf0f0f0f0) >> 4) | ((x & 0x0f0f0f0f) << 4));
            x = (((x & 0xff00ff00) >> 8) | ((x & 0x00ff00ff) << 8));
            return ((x>>16) | (x<<16));
        }
    };


}; // namespace
