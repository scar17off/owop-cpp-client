#pragma once
#include "Chunk.hpp"
#include <unordered_map>
#include <string>

namespace owop {

class World {
public:
    World(const std::string& name);
    
    ChunkPtr getChunk(int x, int y);
    void setPixel(int x, int y, const Color& color);
    Color getPixel(int x, int y) const;
    
    void loadChunk(int x, int y);
    void unloadChunk(int x, int y);
    
    const std::string& getName() const { return name; }

private:
    std::string name;
    std::unordered_map<uint64_t, ChunkPtr> chunks;
    
    static uint64_t makeChunkKey(int x, int y) {
        return (static_cast<uint64_t>(x) << 32) | static_cast<uint32_t>(y);
    }
};

} // namespace owop 