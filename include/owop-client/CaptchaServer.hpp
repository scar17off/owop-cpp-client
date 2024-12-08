#pragma once
#include <memory>
#include <functional>
#include <string>
#include <owop-client/Constants.hpp>

namespace owop {

using CaptchaCallback = std::function<void(const std::string&)>;

class CaptchaServer {
public:
    CaptchaServer(uint16_t port, CaptchaCallback callback);
    ~CaptchaServer();

    void start();
    void stop();
    bool isRunning() const;

private:
    void handleHttpRequest(const std::string& path, const std::string& body, 
                          std::string& responseContent, std::string& contentType);

    class Impl;
    std::unique_ptr<Impl> impl;
    CaptchaCallback captchaCallback;
};

} // namespace owop 