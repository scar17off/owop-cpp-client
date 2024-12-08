#pragma once
#include <cstdint>

namespace owop {

struct Player {
    int32_t x;
    int32_t y;
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t tool;
};

} // namespace owop 