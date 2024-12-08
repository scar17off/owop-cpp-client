#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include "Types.hpp"

namespace owop {
namespace protocol {

// Server -> Client commands
enum class ServerCommand : uint8_t {
    SetId = 0,
    WorldUpdate = 1,
    ChunkLoad = 2,
    Teleport = 3,
    SetRank = 4,
    CaptchaRequest = 5,
    SetWorldName = 6,
    PlayerCount = 7,
    Chat = 8,
    DevChat = 9,
    PlayersLeft = 10,
    PlayersMoved = 11,
    MaxCount = 12,
    DonatorStatus = 13
};

// Client -> Server commands
enum class ClientCommand : uint8_t {
    RequestChunk = 'c',     // 'c' - Request chunk at (x, y)
    Pixel = 'p',           // 'p' - Place pixel
    Move = 'm',            // 'm' - Move to (x, y)
    Chat = 'c',            // 'c' - Send chat message
    CaptchaResponse = 'v', // 'v' - Submit captcha token
    DevChat = 'd'          // 'd' - Send dev chat message
};

// Message structures
struct ChunkData {
    int x;
    int y;
    std::vector<Color> pixels;  // 16x16 pixels
};

struct PixelUpdate {
    int x;
    int y;
    Color color;
    int32_t id;  // Player ID
};

struct PlayerMove {
    int32_t id;
    float x;
    float y;
};

// Helper functions
inline std::vector<uint8_t> makeChunkRequest(int x, int y) {
    std::vector<uint8_t> msg;
    msg.push_back(static_cast<uint8_t>(ClientCommand::RequestChunk));
    // x and y are sent as 32-bit integers
    msg.insert(msg.end(), reinterpret_cast<uint8_t*>(&x), reinterpret_cast<uint8_t*>(&x) + 4);
    msg.insert(msg.end(), reinterpret_cast<uint8_t*>(&y), reinterpret_cast<uint8_t*>(&y) + 4);
    return msg;
}

inline std::vector<uint8_t> makePixelUpdate(int x, int y, const Color& color) {
    std::vector<uint8_t> msg;
    msg.push_back(static_cast<uint8_t>(ClientCommand::Pixel));
    msg.insert(msg.end(), reinterpret_cast<uint8_t*>(&x), reinterpret_cast<uint8_t*>(&x) + 4);
    msg.insert(msg.end(), reinterpret_cast<uint8_t*>(&y), reinterpret_cast<uint8_t*>(&y) + 4);
    msg.push_back(color.r);
    msg.push_back(color.g);
    msg.push_back(color.b);
    return msg;
}

inline std::vector<uint8_t> makeCaptchaResponse(const std::string& token) {
    std::vector<uint8_t> msg;
    msg.push_back(static_cast<uint8_t>(ClientCommand::CaptchaResponse));
    msg.insert(msg.end(), token.begin(), token.end());
    return msg;
}

} // namespace protocol
} // namespace owop 