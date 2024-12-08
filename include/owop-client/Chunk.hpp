#pragma once
#include "Types.hpp"
#include "Constants.hpp"
#include <vector>
#include <memory>

namespace owop {

class Chunk {
public:
    Chunk(int x, int y);

    void setPixel(int x, int y, const Color& color);
    Color getPixel(int x, int y) const;
    
    int getX() const { return x; }
    int getY() const { return y; }
    bool needsRedraw() const { return dirty; }
    void setRedraw(bool value) { dirty = value; }

private:
    int x, y;
    bool dirty;
    std::vector<Color> pixels;
};

using ChunkPtr = std::shared_ptr<Chunk>;

} // namespace owop 