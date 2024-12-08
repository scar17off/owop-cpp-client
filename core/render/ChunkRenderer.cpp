#include <owop-client/render/ChunkRenderer.hpp>
#include <owop-client/Logger.hpp>
#include <cmath>

namespace owop {

ChunkRenderer::ChunkRenderer(GLFWwindow* window)
    : window(window)
{
    // Initialize shader program and other OpenGL resources here
}

ChunkRenderer::~ChunkRenderer() {
    // Clean up textures
    for (auto& pair : chunks) {
        if (pair.second.texture != 0) {
            glDeleteTextures(1, &pair.second.texture);
        }
    }
}

void ChunkRenderer::updateChunk(int x, int y, const std::vector<Color>& pixels) {
    // Convert world coordinates to chunk coordinates
    int chunkX = static_cast<int>(std::floor(static_cast<float>(x) / 16.0f));
    int chunkY = static_cast<int>(std::floor(static_cast<float>(y) / 16.0f));
    
    uint64_t key = getChunkKey(chunkX, chunkY);
    auto& chunk = chunks[key];
    
    // Update chunk data
    chunk.pixels = pixels;
    chunk.dirty = true;

    Logger::info("ChunkRenderer", "Updated chunk at (" + std::to_string(chunkX) + ", " + 
        std::to_string(chunkY) + ") with " + std::to_string(pixels.size()) + " pixels");
}

void ChunkRenderer::updateChunkTexture(Chunk& chunk) {
    if (!chunk.dirty) return;

    // Create texture if it doesn't exist
    if (chunk.texture == 0) {
        glGenTextures(1, &chunk.texture);
        glBindTexture(GL_TEXTURE_2D, chunk.texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

    // Update texture data
    glBindTexture(GL_TEXTURE_2D, chunk.texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 16, 16, 0, GL_RGB, GL_UNSIGNED_BYTE, chunk.pixels.data());
    
    chunk.dirty = false;
}

void ChunkRenderer::render(const Camera& camera, int windowWidth, int windowHeight) {
    glClear(GL_COLOR_BUFFER_BIT);
    
    // Set up orthographic projection
    float zoom = camera.getZoom();
    float left = camera.getX() - (windowWidth / 2.0f) / zoom;
    float right = camera.getX() + (windowWidth / 2.0f) / zoom;
    float bottom = camera.getY() + (windowHeight / 2.0f) / zoom;
    float top = camera.getY() - (windowHeight / 2.0f) / zoom;

    // Enable texturing and blending
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Render each chunk
    for (auto& pair : chunks) {
        // Update texture if needed
        updateChunkTexture(pair.second);

        // Calculate chunk position
        int chunkX = static_cast<int>(pair.first >> 32);
        int chunkY = static_cast<int>(pair.first & 0xFFFFFFFF);
        
        float worldX = chunkX * 16.0f;
        float worldY = chunkY * 16.0f;

        // Skip chunks outside view
        if (worldX + 16 < left || worldX > right || 
            worldY + 16 < top || worldY > bottom) {
            continue;
        }

        // Bind texture
        glBindTexture(GL_TEXTURE_2D, pair.second.texture);

        // Draw textured quad
        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex2f(worldX, worldY);
        glTexCoord2f(1.0f, 0.0f); glVertex2f(worldX + 16, worldY);
        glTexCoord2f(1.0f, 1.0f); glVertex2f(worldX + 16, worldY + 16);
        glTexCoord2f(0.0f, 1.0f); glVertex2f(worldX, worldY + 16);
        glEnd();
    }

    // Cleanup state
    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
}

void ChunkRenderer::setPixel(int x, int y, const Color& color) {
    // Convert world coordinates to chunk coordinates
    int chunkX = static_cast<int>(std::floor(static_cast<float>(x) / 16.0f));
    int chunkY = static_cast<int>(std::floor(static_cast<float>(y) / 16.0f));
    
    uint64_t key = getChunkKey(chunkX, chunkY);
    auto& chunk = chunks[key];

    // Calculate pixel position within chunk
    int localX = x - (chunkX * 16);
    int localY = y - (chunkY * 16);
    int pixelIndex = (localY * 16 + localX);

    // Update pixel if within bounds
    if (pixelIndex >= 0 && pixelIndex < 256 && chunk.pixels.size() == 256) {
        chunk.pixels[pixelIndex] = color;
        chunk.dirty = true;
    }
}

} // namespace owop