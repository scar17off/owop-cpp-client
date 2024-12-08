#include <owop-client/Settings.hpp>
#include <owop-client/Logger.hpp>
#include <fstream>
#include <filesystem>
#include <nlohmann/json.hpp>

namespace owop {

void Settings::save() {
    try {
        nlohmann::json j;
        j["serverDomain"] = serverDomain;
        j["worldName"] = worldName;
        j["requireCaptcha"] = requireCaptcha;

        std::filesystem::path settingsPath = "settings.json";
        std::ofstream file(settingsPath);
        if (file.is_open()) {
            file << j.dump(4);
            Logger::info("Settings", "Settings saved successfully");
        } else {
            Logger::error("Settings", "Failed to open settings file for writing");
        }
    } catch (const std::exception& e) {
        Logger::error("Settings", "Error saving settings: " + std::string(e.what()));
    }
}

void Settings::load() {
    try {
        std::filesystem::path settingsPath = "settings.json";
        if (!std::filesystem::exists(settingsPath)) {
            Logger::info("Settings", "No settings file found, using defaults");
            return;
        }

        std::ifstream file(settingsPath);
        if (file.is_open()) {
            nlohmann::json j;
            file >> j;

            if (j.contains("serverDomain")) serverDomain = j["serverDomain"].get<std::string>();
            if (j.contains("worldName")) worldName = j["worldName"].get<std::string>();
            if (j.contains("requireCaptcha")) requireCaptcha = j["requireCaptcha"].get<bool>();

            Logger::info("Settings", "Settings loaded successfully");
        } else {
            Logger::error("Settings", "Failed to open settings file for reading");
        }
    } catch (const std::exception& e) {
        Logger::error("Settings", "Error loading settings: " + std::string(e.what()));
    }
}

} // namespace owop 