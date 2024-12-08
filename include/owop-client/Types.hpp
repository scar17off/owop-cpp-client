#pragma once
#include <cstdint>

namespace owop {

struct Color {
    uint8_t r, g, b;
    
    Color() : r(0), g(0), b(0) {}
    Color(uint8_t r, uint8_t g, uint8_t b) : r(r), g(g), b(b) {}
};

struct Vec2 {
    float x, y;
    
    Vec2() : x(0), y(0) {}
    Vec2(float x, float y) : x(x), y(y) {}
};

struct Vec2i {
    int x, y;
    
    Vec2i() : x(0), y(0) {}
    Vec2i(int x, int y) : x(x), y(y) {}
};

} // namespace owop 