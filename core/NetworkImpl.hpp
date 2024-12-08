#pragma once
#include <owop-client/WebSocketIncludes.hpp>
#include <owop-client/CaptchaServer.hpp>
#include <owop-client/Types.hpp>
#include <owop-client/Player.hpp>
#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <thread>
#include <queue>
#include <set>
#include <mutex>
#include <atomic>

namespace owop {

// Helper struct for chunk coordinates
struct ChunkCoord {
    int32_t x;
    int32_t y;

    bool operator==(const ChunkCoord& other) const {
        return x == other.x && y == other.y;
    }

    bool operator<(const ChunkCoord& other) const {
        return x < other.x || (x == other.x && y < other.y);
    }
};

class NetworkImpl {
public:
    NetworkImpl(std::unique_ptr<CaptchaServer>* captchaServer);
    ~NetworkImpl() {
        disconnect();  // Ensure clean shutdown
    }

    void connect(const std::string& url, const std::string& world);
    void disconnect();
    void submitCaptcha(const std::string& token);
    void requestChunksInView(int32_t centerX, int32_t centerY, float zoom);
    bool isWaitingForCaptcha() const { return waitingForCaptcha; }
    const std::unordered_map<uint32_t, Player>& getPlayers() const { return players; }

    void setChunkDataCallback(std::function<void(int, int, const std::vector<Color>&)> callback) {
        chunkDataCallback = callback;
    }

private:
    void handleMessage(const std::string& payload);
    void requestChunk(int32_t x, int32_t y);
    void sendBinary(const uint8_t* data, size_t length);
    void sendWorldJoinMessage();
    void attemptConnection();
    void processNextChunk();
    bool isChunkQueued(const ChunkCoord& coord) const;

    WebSocketClient client;
    WebSocketConnection connection;
    bool connected{false};
    bool connecting{false};
    bool waitingForCaptcha{false};
    std::string worldName;
    std::string serverUrl;
    std::unique_ptr<CaptchaServer>* captchaServer;
    std::string pendingToken;
    uint32_t playerId{0};
    std::unordered_map<uint32_t, Player> players;
    std::function<void(int, int, const std::vector<Color>&)> chunkDataCallback;
    std::thread websocketThread;

    // Chunk management
    std::queue<ChunkCoord> chunkRequestQueue;
    std::set<ChunkCoord> loadedChunks;
    std::set<ChunkCoord> pendingChunks;  // Chunks that have been requested but not yet received
    std::mutex chunkMutex;
    std::atomic<bool> waitingForChunk{false};  // True if waiting for a chunk response
}; 

} // namespace owop