#include <owop-client/Network.hpp>
#include <owop-client/Logger.hpp>
#include <owop-client/Player.hpp>
#include <owop-client/Types.hpp>
#include <owop-client/Settings.hpp>
#include "NetworkImpl.hpp"

namespace owop {

Network::Network() 
    : captchaServer(std::make_unique<CaptchaServer>(8081, 
        [this](const std::string& token) {
            if (impl) {
                impl->submitCaptcha(token);
            }
        }))
    , impl(std::make_unique<NetworkImpl>(&captchaServer))
    , running(false)
{
}

Network::~Network() {
    disconnect();
}

void Network::connect(const std::string& url, const std::string& worldName) {
    if (!impl) return;
    impl->connect(url, worldName);
}

void Network::disconnect() {
    running = false;
    
    if (impl) {
        impl->disconnect();
    }
    
    if (networkThread.joinable()) {
        networkThread.join();
    }
    
    if (captchaServer) {
        captchaServer->stop();
    }
}

void Network::submitCaptcha(const std::string& token) {
    if (!impl) return;
    impl->submitCaptcha(token);
}

void Network::requestChunksInView(int32_t centerX, int32_t centerY, float zoom) {
    if (!impl) return;
    impl->requestChunksInView(centerX, centerY, zoom);
}

bool Network::isWaitingForCaptcha() const {
    if (!impl) return false;
    return impl->isWaitingForCaptcha();
}

void Network::setChunkDataCallback(std::function<void(int, int, const std::vector<Color>&)> callback) {
    if (!impl) return;
    impl->setChunkDataCallback(callback);
}

const std::unordered_map<uint32_t, Player>& Network::getPlayers() const {
    static std::unordered_map<uint32_t, Player> empty;
    if (!impl) return empty;
    return impl->getPlayers();
}

} // namespace owop