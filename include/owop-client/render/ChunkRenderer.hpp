#pragma once
#include "../Types.hpp"
#include "../Camera.hpp"
#include <glad/glad.h>
#include <vector>
#include <unordered_map>
#include <GLFW/glfw3.h>

namespace owop {

class ChunkRenderer {
public:
    ChunkRenderer(GLFWwindow* window);
    ~ChunkRenderer();

    void render(const Camera& camera, int windowWidth, int windowHeight);
    void updateChunk(int x, int y, const std::vector<Color>& pixels);
    void setPixel(int x, int y, const Color& color);

private:
    GLFWwindow* window;
    
    struct Chunk {
        GLuint texture = 0;
        bool dirty = false;
        std::vector<Color> pixels;
    };

    std::unordered_map<uint64_t, Chunk> chunks;

    void updateChunkTexture(Chunk& chunk);
    uint64_t getChunkKey(int x, int y) const {
        return (static_cast<uint64_t>(x) << 32) | static_cast<uint32_t>(y);
    }
};

} // namespace owop 