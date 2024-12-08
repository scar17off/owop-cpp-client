#pragma once
#include <string>
#include <functional>
#include <thread>
#include <boost/asio.hpp>
#include <boost/beast.hpp>

namespace owop {

class CaptchaServer {
public:
    using TokenCallback = std::function<void(const std::string&)>;

    CaptchaServer(int port = 8080);
    ~CaptchaServer();

    void start();
    void stop();
    void setTokenCallback(TokenCallback callback) { onToken = callback; }

private:
    int port;
    bool running;
    std::thread serverThread;
    TokenCallback onToken;
    boost::asio::io_context ioc;
    
    void run();
    void handleRequest(boost::beast::http::request<boost::beast::http::string_body>& req);
};

} // namespace owop 