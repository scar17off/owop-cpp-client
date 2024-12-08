#pragma once
#include <string>

namespace owop {

class Settings {
public:
    static Settings& getInstance() {
        static Settings instance;
        return instance;
    }

    // Server settings
    std::string serverDomain = "ourworldofpixels.com";
    std::string worldName = "main";
    bool requireCaptcha = true;

    // Save/Load settings
    void save();
    void load();

private:
    Settings() = default;
    Settings(const Settings&) = delete;
    Settings& operator=(const Settings&) = delete;
};

} // namespace owop