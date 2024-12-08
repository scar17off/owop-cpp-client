#pragma once
#include <string>
#include <functional>
#include <thread>
#include <atomic>
#include <memory>
#include <unordered_map>
#include "Types.hpp"
#include "CaptchaServer.hpp"
#include "Player.hpp"
#include "Settings.hpp"

namespace owop {

class NetworkImpl;

class Network {
public:
    Network();
    ~Network();

    void connect(const std::string& url, const std::string& worldName);
    void disconnect();
    void submitCaptcha(const std::string& token);
    const std::unordered_map<uint32_t, Player>& getPlayers() const;
    void requestChunksInView(int32_t centerX, int32_t centerY, float zoom);
    bool isWaitingForCaptcha() const;

    void setChunkDataCallback(std::function<void(int, int, const std::vector<Color>&)> callback);
    void setPixelUpdateCallback(std::function<void(int, int, const Color&)> callback);

private:
    void handleWorldData(const std::string& data);
    void handleChunkData(const std::string& data);
    void runNetworkLoop();

    std::function<void(int, int, const std::vector<Color>&)> chunkDataCallback;
    std::function<void(int, int, const Color&)> pixelUpdateCallback;
    std::thread networkThread;
    std::atomic<bool> running{false};
    std::unique_ptr<CaptchaServer> captchaServer;
    std::unique_ptr<NetworkImpl> impl;
};

} // namespace owop 