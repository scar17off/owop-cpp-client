#pragma once
#include <iostream>
#include <string>
#include <ctime>

namespace owop {

class Logger {
public:
    static void debug(const std::string& component, const std::string& message) {
        log("DEBUG", component, message);
    }

    static void info(const std::string& component, const std::string& message) {
        log("INFO", component, message);
    }

    static void warning(const std::string& component, const std::string& message) {
        log("WARNING", component, message);
    }

    static void error(const std::string& component, const std::string& message) {
        log("ERROR", component, message);
    }

private:
    static void log(const std::string& level, const std::string& component, const std::string& message) {
        time_t now = time(nullptr);
        char timestamp[64];
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));
        std::cout << "[" << timestamp << "] [" << level << "] [" << component << "] " << message << std::endl;
    }
};

} // namespace owop 