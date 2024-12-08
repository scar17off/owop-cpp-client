#pragma once

namespace owop {

// World constants
constexpr int CHUNK_SIZE = 16;
constexpr int CLUSTER_CHUNK_AMOUNT = 8;

// Camera constants
constexpr float DEFAULT_ZOOM = 16.0f;
constexpr float MIN_ZOOM = 1.0f;
constexpr float MAX_ZOOM = 32.0f;

// Network constants
constexpr const char* DEFAULT_SERVER = "wss://9060b3b6-0e87-42d2-93e3-2219d6422023-00-yo1d43p3n3x5.picard.replit.dev";
constexpr const char* RECAPTCHA_SITE_KEY = "6LcgvScUAAAAAARUXtwrM8MP0A0N70z4DHNJh-KI";
constexpr const char* RECAPTCHA_SCRIPT = R"(
<script src='https://www.google.com/recaptcha/api.js' async defer></script>
<div class="g-recaptcha" 
     data-sitekey="6LcgvScUAAAAAARUXtwrM8MP0A0N70z4DHNJh-KI"
     data-callback="onCaptchaCompleted"></div>
)";

// Protocol constants
constexpr const char* PROTOCOL_VERSION = "0"; // Protocol version
constexpr const char* DEFAULT_WORLD = "main"; // Default world name

// Tool IDs
enum class Tool {
    Cursor,
    Move,
    Pipette
};

// Ranks
enum class Rank {
    None = 0,
    User = 1,
    Moderator = 2,
    Admin = 3
};

} // namespace owop 