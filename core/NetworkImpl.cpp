#include "NetworkImpl.hpp"
#include <owop-client/Logger.hpp>
#include <owop-client/Settings.hpp>
#include <cctype>

namespace owop {

NetworkImpl::NetworkImpl(std::unique_ptr<CaptchaServer>* captchaServer)
    : captchaServer(captchaServer)
    , connected(false)
    , connecting(false)
    , waitingForCaptcha(false)
{
    // Initialize WebSocket client
    client.clear_access_channels(websocketpp::log::alevel::all);
    client.clear_error_channels(websocketpp::log::elevel::all);
    client.set_access_channels(websocketpp::log::alevel::connect);
    client.set_access_channels(websocketpp::log::alevel::disconnect);
    client.set_access_channels(websocketpp::log::alevel::app);
    client.set_error_channels(websocketpp::log::elevel::fatal);

    client.init_asio();

    // Set up TLS
    client.set_tls_init_handler([](websocketpp::connection_hdl) {
        auto ctx = websocketpp::lib::make_shared<boost::asio::ssl::context>(
            boost::asio::ssl::context::tlsv12);
        ctx->set_verify_mode(boost::asio::ssl::verify_none);
        return ctx;
    });

    // Set up callbacks
    client.set_open_handler([this](WebSocketConnection hdl) {
        Logger::info("Network", "WebSocket connected");
        connection = hdl;
        connected = true;
        connecting = false;

        // If we have a pending token, send it now
        if (!pendingToken.empty()) {
            Logger::info("Network", "Sending pending captcha token...");
            try {
                auto con = client.get_con_from_hdl(connection);
                std::string tokenMsg = "CaptchA" + pendingToken;
                con->send(tokenMsg);
                Logger::info("Network", "Sent captcha token");
            } catch (const std::exception& e) {
                Logger::error("Network", "Failed to send token: " + std::string(e.what()));
            }
            pendingToken.clear();
        }
    });

    client.set_close_handler([this](WebSocketConnection hdl) {
        auto con = client.get_con_from_hdl(hdl);
        std::string reason = con->get_remote_close_reason();
        Logger::info("Network", "WebSocket disconnected - Reason: " + reason);
        
        connected = false;
        connecting = false;
        connection.reset();
        pendingToken.clear();
    });

    client.set_message_handler([this](WebSocketConnection hdl, WebSocketClient::message_ptr msg) {
        try {
            handleMessage(msg->get_payload());
        } catch (const std::exception& e) {
            Logger::error("Network", "Error handling message: " + std::string(e.what()));
        }
    });

    client.set_fail_handler([this](WebSocketConnection hdl) {
        auto con = client.get_con_from_hdl(hdl);
        Logger::error("Network", "Connection failed: " + con->get_ec().message());
        
        connected = false;
        connecting = false;
        connection.reset();
        pendingToken.clear();
    });
}

void NetworkImpl::connect(const std::string& url, const std::string& world) {
    if (connecting || connected) {
        Logger::info("Network", "Already connecting/connected - skipping connect");
        return;
    }
    
    worldName = world;
    serverUrl = url;
    connecting = true;
    waitingForCaptcha = true;  // Always start with captcha check
    
    // Clear chunk state
    {
        std::lock_guard<std::mutex> lock(chunkMutex);
        std::queue<ChunkCoord>().swap(chunkRequestQueue);  // Clear queue
        loadedChunks.clear();
        pendingChunks.clear();
        waitingForChunk = false;
    }
    
    // Start captcha server if not already running
    if (captchaServer && *captchaServer && !(*captchaServer)->isRunning()) {
        Logger::info("Network", "Starting captcha server...");
        (*captchaServer)->start();
    }

    // Initialize WebSocket if needed
    if (!client.is_listening()) {
        client.reset();
        client.start_perpetual();
        
        // Run the ASIO io_service in a separate thread
        websocketThread = std::thread([this]() {
            try {
                client.run();
            } catch (const std::exception& e) {
                Logger::error("Network", "WebSocket thread error: " + std::string(e.what()));
            }
        });
    }

    // Attempt connection
    attemptConnection();
}

void NetworkImpl::attemptConnection() {
    try {
        // Create new connection
        websocketpp::lib::error_code ec;
        auto con = client.get_connection(serverUrl, ec);
        
        if (ec) {
            Logger::error("Network", "Could not create connection: " + ec.message());
            connecting = false;
            return;
        }

        // Set headers
        con->append_header("Origin", "https://ourworldofpixels.com");
        con->append_header("User-Agent", "Mozilla/5.0");
        con->append_header("Pragma", "no-cache");
        con->append_header("Cache-Control", "no-cache");

        // Connect
        Logger::info("Network", "Connecting to " + serverUrl);
        client.connect(con);
    } catch (const std::exception& e) {
        Logger::error("Network", "Failed to connect: " + std::string(e.what()));
        connecting = false;
    }
}

void NetworkImpl::disconnect() {
    if (!connected && !connecting) return;

    Logger::info("Network", "Disconnecting...");
    
    // Clear chunk state
    {
        std::lock_guard<std::mutex> lock(chunkMutex);
        std::queue<ChunkCoord>().swap(chunkRequestQueue);
        loadedChunks.clear();
        pendingChunks.clear();
        waitingForChunk = false;
    }
    
    // Stop the captcha server first
    if (captchaServer && *captchaServer && (*captchaServer)->isRunning()) {
        (*captchaServer)->stop();
    }

    // Close WebSocket connection if active
    if (connection.lock()) {
        try {
            auto con = client.get_con_from_hdl(connection);
            if (con && con->get_state() == websocketpp::session::state::open) {
                con->close(websocketpp::close::status::normal, "Client disconnecting");
            }
        } catch (const std::exception& e) {
            Logger::error("Network", "Error during disconnect: " + std::string(e.what()));
        }
    }

    // Reset state
    connecting = false;
    connected = false;
    waitingForCaptcha = false;
    connection.reset();
    pendingToken.clear();

    // Stop WebSocket client if it's running
    if (client.is_listening()) {
        client.stop_perpetual();
        if (websocketThread.joinable()) {
            websocketThread.join();
        }
    }
}

void NetworkImpl::submitCaptcha(const std::string& token) {
    Logger::info("Network", "Received captcha token");
    
    if (!token.empty()) {
        pendingToken = token;
        waitingForCaptcha = false;
        
        if (connected) {
            // Send token directly
            try {
                auto con = client.get_con_from_hdl(connection);
                std::string tokenMsg = "CaptchA" + token;
                con->send(tokenMsg);
                Logger::info("Network", "Sent captcha token");
            } catch (const std::exception& e) {
                Logger::error("Network", "Failed to send token: " + std::string(e.what()));
            }
            pendingToken.clear();
        } else if (!connecting) {
            // If not connecting, attempt connection
            Logger::info("Network", "Stored captcha token and attempting connection");
            attemptConnection();
        } else {
            Logger::info("Network", "Stored captcha token for when connection is established");
        }
    } else {
        Logger::error("Network", "Received empty captcha token");
    }
}

bool NetworkImpl::isChunkQueued(const ChunkCoord& coord) const {
    // Check if the chunk is in the queue
    auto queue = chunkRequestQueue;
    while (!queue.empty()) {
        if (queue.front() == coord) return true;
        queue.pop();
    }
    return false;
}

void NetworkImpl::requestChunksInView(int32_t centerX, int32_t centerY, float zoom) {
    if (!connected) return;

    try {
        // Calculate visible area in chunks (16x16 pixels per chunk)
        const int32_t CHUNK_SIZE = 16;
        const int32_t viewportWidth = 800;  // TODO: Get actual viewport size
        const int32_t viewportHeight = 600;
        
        // Convert world coordinates to chunk coordinates
        int32_t centerChunkX = static_cast<int32_t>(std::floor(static_cast<float>(centerX) / CHUNK_SIZE));
        int32_t centerChunkY = static_cast<int32_t>(std::floor(static_cast<float>(centerY) / CHUNK_SIZE));
        
        // Calculate how many chunks we can see based on zoom level
        int32_t visibleChunksX = std::max(1, static_cast<int32_t>((viewportWidth / zoom) / CHUNK_SIZE) + 1);
        int32_t visibleChunksY = std::max(1, static_cast<int32_t>((viewportHeight / zoom) / CHUNK_SIZE) + 1);

        // Limit the number of chunks to request at once
        visibleChunksX = std::min(visibleChunksX, 16);
        visibleChunksY = std::min(visibleChunksY, 16);

        bool shouldProcessNext = false;
        {
            std::lock_guard<std::mutex> lock(chunkMutex);

            // Add visible chunks to request queue if not already loaded or pending
            for (int32_t y = centerChunkY - visibleChunksY; y <= centerChunkY + visibleChunksY; y++) {
                for (int32_t x = centerChunkX - visibleChunksX; x <= centerChunkX + visibleChunksX; x++) {
                    ChunkCoord coord{x, y};
                    if (loadedChunks.find(coord) == loadedChunks.end() && 
                        pendingChunks.find(coord) == pendingChunks.end() &&
                        !isChunkQueued(coord)) {
                        chunkRequestQueue.push(coord);
                    }
                }
            }

            // Check if we should process next chunk
            shouldProcessNext = !waitingForChunk && !chunkRequestQueue.empty();
        }

        // Process next chunk outside of mutex lock if needed
        if (shouldProcessNext) {
            processNextChunk();
        }
    } catch (const std::exception& e) {
        Logger::error("Network", "Error in requestChunksInView: " + std::string(e.what()));
    }
}

void NetworkImpl::processNextChunk() {
    if (!connected) return;

    try {
        ChunkCoord coord;
        bool shouldRequest = false;
        
        {
            std::lock_guard<std::mutex> lock(chunkMutex);
            
            if (chunkRequestQueue.empty() || waitingForChunk) {
                return;
            }

            coord = chunkRequestQueue.front();
            chunkRequestQueue.pop();

            // Skip if already loaded or pending
            if (loadedChunks.find(coord) == loadedChunks.end() &&
                pendingChunks.find(coord) == pendingChunks.end()) {
                pendingChunks.insert(coord);
                waitingForChunk = true;
                shouldRequest = true;
            }
        }

        // Request the chunk outside of mutex lock if needed
        if (shouldRequest) {
            requestChunk(coord.x, coord.y);
        }
    } catch (const std::exception& e) {
        Logger::error("Network", "Error in processNextChunk: " + std::string(e.what()));
        std::lock_guard<std::mutex> lock(chunkMutex);
        waitingForChunk = false;  // Reset state on error
    }
}

void NetworkImpl::requestChunk(int32_t x, int32_t y) {
    if (!connected) return;

    try {
        // Create chunk request message
        std::vector<uint8_t> message(9);
        message[0] = 0x02;  // chunkLoad opcode
        
        // Write coordinates in little-endian format
        std::memcpy(&message[1], &x, sizeof(int32_t));
        std::memcpy(&message[5], &y, sizeof(int32_t));

        auto con = client.get_con_from_hdl(connection);
        if (con && con->get_state() == websocketpp::session::state::open) {
            con->send(message.data(), message.size(), websocketpp::frame::opcode::binary);
            Logger::info("Network", "Requested chunk at (" + std::to_string(x) + ", " + std::to_string(y) + ")");
        } else {
            Logger::error("Network", "Failed to request chunk: connection not ready");
            std::lock_guard<std::mutex> lock(chunkMutex);
            pendingChunks.erase(ChunkCoord{x, y});
            waitingForChunk = false;
        }
    } catch (const std::exception& e) {
        Logger::error("Network", "Error requesting chunk: " + std::string(e.what()));
        std::lock_guard<std::mutex> lock(chunkMutex);
        pendingChunks.erase(ChunkCoord{x, y});
        waitingForChunk = false;
    }
}

void NetworkImpl::sendBinary(const uint8_t* data, size_t length) {
    if (!connected || !connection.lock()) return;
    
    try {
        auto con = client.get_con_from_hdl(connection);
        if (con) {
            con->send(data, length, websocketpp::frame::opcode::binary);
        }
    } catch (const std::exception& e) {
        Logger::error("Network", "Failed to send binary data: " + std::string(e.what()));
        // Don't throw here - we want to handle send errors gracefully
    }
}

void NetworkImpl::sendWorldJoinMessage() {
    if (!connected) return;

    Logger::info("Network", "Sending world join message for world: " + worldName);
    
    // Convert world name to bytes, keeping only valid characters
    std::vector<uint8_t> worldBytes;
    for (char c : worldName) {
        c = std::tolower(c);
        if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '.') {
            worldBytes.push_back(static_cast<uint8_t>(c));
        }
    }

    // Limit world name length to 24 bytes
    if (worldBytes.size() > 24) {
        worldBytes.resize(24);
    }

    // Add verification bytes (25565 in little-endian)
    worldBytes.push_back(0xDD);  // 25565 & 0xFF
    worldBytes.push_back(0x63);  // (25565 >> 8) & 0xFF

    try {
        auto con = client.get_con_from_hdl(connection);
        if (con) {
            con->send(worldBytes.data(), worldBytes.size(), websocketpp::frame::opcode::binary);
            Logger::info("Network", "Sent world join message");
        }
    } catch (const std::exception& e) {
        Logger::error("Network", "Failed to send world join message: " + std::string(e.what()));
    }
}

void NetworkImpl::handleMessage(const std::string& payload) {
    if (payload.empty()) return;

    try {
        // First byte is opcode
        uint8_t opcode = static_cast<uint8_t>(payload[0]);
        
        switch (opcode) {
            case 0: { // setId
                if (payload.length() < 5) return;
                playerId = *reinterpret_cast<const uint32_t*>(payload.data() + 1);
                Logger::info("Network", "Received player ID: " + std::to_string(playerId));
                break;
            }
            case 1: { // worldUpdate
                if (payload.length() < 2) return;
                
                // Read player count
                uint8_t playerCount = static_cast<uint8_t>(payload[1]);
                size_t offset = 2;

                // Process player updates
                for (uint8_t i = 0; i < playerCount; i++) {
                    if (offset + 16 > payload.length()) break;

                    uint32_t pid = *reinterpret_cast<const uint32_t*>(payload.data() + offset);
                    if (pid != playerId) {  // Skip our own updates
                        int32_t x = *reinterpret_cast<const int32_t*>(payload.data() + offset + 4);
                        int32_t y = *reinterpret_cast<const int32_t*>(payload.data() + offset + 8);
                        uint8_t r = static_cast<uint8_t>(payload[offset + 12]);
                        uint8_t g = static_cast<uint8_t>(payload[offset + 13]);
                        uint8_t b = static_cast<uint8_t>(payload[offset + 14]);
                        uint8_t tool = static_cast<uint8_t>(payload[offset + 15]);

                        Player& player = players[pid];
                        player.x = x;
                        player.y = y;
                        player.r = r;
                        player.g = g;
                        player.b = b;
                        player.tool = tool;
                    }
                    offset += 16;
                }

                // Process pixel updates
                if (offset + 2 <= payload.length()) {
                    uint16_t pixelCount = *reinterpret_cast<const uint16_t*>(payload.data() + offset);
                    offset += 2;

                    for (uint16_t i = 0; i < pixelCount; i++) {
                        if (offset + 15 > payload.length()) break;

                        uint32_t id = *reinterpret_cast<const uint32_t*>(payload.data() + offset);
                        int32_t x = *reinterpret_cast<const int32_t*>(payload.data() + offset + 4);
                        int32_t y = *reinterpret_cast<const int32_t*>(payload.data() + offset + 8);
                        uint8_t r = static_cast<uint8_t>(payload[offset + 12]);
                        uint8_t g = static_cast<uint8_t>(payload[offset + 13]);
                        uint8_t b = static_cast<uint8_t>(payload[offset + 14]);

                        // TODO: Handle pixel updates
                        offset += 15;
                    }
                }

                // Process disconnects
                if (offset + 1 <= payload.length()) {
                    uint8_t disconnectCount = static_cast<uint8_t>(payload[offset++]);
                    for (uint8_t i = 0; i < disconnectCount; i++) {
                        if (offset + 4 > payload.length()) break;
                        uint32_t pid = *reinterpret_cast<const uint32_t*>(payload.data() + offset);
                        players.erase(pid);
                        offset += 4;
                    }
                }
                break;
            }
            case 2: { // chunkLoad
                if (payload.length() < 778) { // 1 byte opcode + 4 bytes x + 4 bytes y + 1 byte locked + 768 bytes data
                    Logger::error("Network", "Invalid chunk data length: " + std::to_string(payload.length()));
                    return;
                }

                // Extract chunk coordinates (4 bytes each)
                int32_t chunkX = *reinterpret_cast<const int32_t*>(payload.data() + 1);
                int32_t chunkY = *reinterpret_cast<const int32_t*>(payload.data() + 5);
                uint8_t locked = static_cast<uint8_t>(payload[9]);

                // Extract chunk data (16x16x3 = 768 bytes)
                std::vector<Color> chunkData;
                chunkData.reserve(256); // 16x16 pixels

                const uint8_t* data = reinterpret_cast<const uint8_t*>(payload.data() + 10);
                for (size_t i = 0; i < 768; i += 3) {
                    Color pixel;
                    pixel.r = data[i];
                    pixel.g = data[i + 1];
                    pixel.b = data[i + 2];
                    chunkData.push_back(pixel);
                }

                bool shouldProcessNext = false;
                {
                    std::lock_guard<std::mutex> lock(chunkMutex);
                    ChunkCoord coord{chunkX, chunkY};
                    loadedChunks.insert(coord);
                    pendingChunks.erase(coord);
                    waitingForChunk = false;
                    shouldProcessNext = !chunkRequestQueue.empty();
                }

                // Call the callback outside of mutex lock
                if (chunkDataCallback) {
                    chunkDataCallback(chunkX, chunkY, chunkData);
                }

                // Process next chunk if needed
                if (shouldProcessNext) {
                    processNextChunk();
                }

                Logger::info("Network", "Received chunk data for (" + std::to_string(chunkX) + ", " + 
                    std::to_string(chunkY) + ")");
                break;
            }
            case 3: { // teleport
                if (payload.length() < 9) return;
                int32_t x = *reinterpret_cast<const int32_t*>(payload.data() + 1);
                int32_t y = *reinterpret_cast<const int32_t*>(payload.data() + 5);
                // TODO: Handle teleport
                break;
            }
            case 4: { // setRank
                if (payload.length() < 2) return;
                uint8_t rank = static_cast<uint8_t>(payload[1]);
                // TODO: Handle rank update
                break;
            }
            case 5: { // captcha
                if (payload.length() < 2) return;
                uint8_t captchaState = static_cast<uint8_t>(payload[1]);
                
                switch (captchaState) {
                    case 0: // CA_WAITING
                        Logger::info("Network", "Received captcha state: WAITING (0)");
                        waitingForCaptcha = true;
                        break;
                    case 1: // CA_VERIFYING
                        Logger::info("Network", "Received captcha state: VERIFYING (1)");
                        break;
                    case 2: // CA_VERIFIED
                        Logger::info("Network", "Received captcha state: VERIFIED (2)");
                        waitingForCaptcha = false;
                        break;
                    case 3: // CA_OK
                        Logger::info("Network", "Received captcha state: OK (3)");
                        waitingForCaptcha = false;
                        // Send world join message after successful captcha
                        sendWorldJoinMessage();
                        break;
                    case 4: // CA_INVALID
                        Logger::info("Network", "Received captcha state: INVALID (4)");
                        waitingForCaptcha = true;
                        break;
                }
                break;
            }
            case 6: { // setPQuota
                // TODO: Handle pixel quota update
                break;
            }
            case 7: { // chunkProtected
                if (payload.length() < 10) return;
                int32_t x = *reinterpret_cast<const int32_t*>(payload.data() + 1);
                int32_t y = *reinterpret_cast<const int32_t*>(payload.data() + 5);
                uint8_t state = static_cast<uint8_t>(payload[9]);
                // TODO: Handle chunk protection update
                break;
            }
        }
    } catch (const std::exception& e) {
        Logger::error("Network", "Error handling message: " + std::string(e.what()));
        
        // Reset chunk state on error
        std::lock_guard<std::mutex> lock(chunkMutex);
        waitingForChunk = false;
    }
}

} // namespace owop 