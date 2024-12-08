#include <owop-client/CaptchaServer.hpp>
#include <owop-client/Logger.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <thread>

namespace owop {

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

class CaptchaServer::Impl {
public:
    bool running{false};
    
    Impl(uint16_t port, CaptchaCallback callback)
        : port(port)
        , callback(callback)
        , ioc()
        , acceptor(nullptr)
        , running(false)
    {
        Logger::info("CaptchaServer", "Created on port " + std::to_string(port));
    }

    void start() {
        if (running) {
            Logger::info("CaptchaServer", "Server already running");
            return;
        }

        Logger::info("CaptchaServer", "Starting server...");
        running = true;

        try {
            // Create and configure the acceptor
            acceptor = std::make_unique<tcp::acceptor>(ioc, tcp::endpoint(tcp::v4(), port));
            Logger::info("CaptchaServer", "Successfully bound to port " + std::to_string(port));

            // Start the server thread
            serverThread = std::thread([this]() {
                Logger::info("CaptchaServer", "Server thread started");
                Logger::info("CaptchaServer", "Server thread running on port " + std::to_string(port));
                
                while (running) {
                    try {
                        Logger::info("CaptchaServer", "Waiting for connection...");
                        
                        // Create a new socket for the connection
                        auto socket = std::make_shared<tcp::socket>(ioc);
                        
                        // Accept connection
                        boost::system::error_code ec;
                        acceptor->accept(*socket, ec);
                        
                        if (ec) {
                            if (running) {  // Only log error if we're still supposed to be running
                                Logger::error("CaptchaServer", "Error accepting connection: " + ec.message());
                            }
                            continue;
                        }

                        // Handle the connection in a separate thread
                        std::thread([this, socket]() {
                            handleConnection(socket);
                        }).detach();

                    } catch (const std::exception& e) {
                        if (running) {  // Only log error if we're still supposed to be running
                            Logger::error("CaptchaServer", "Error handling request: " + std::string(e.what()));
                        }
                    }
                }

                Logger::info("CaptchaServer", "Server thread stopping");
            });

        } catch (const std::exception& e) {
            Logger::error("CaptchaServer", "Failed to start server: " + std::string(e.what()));
            running = false;
            if (serverThread.joinable()) {
                serverThread.join();
            }
        }
    }

    void stop() {
        if (!running) return;

        Logger::info("CaptchaServer", "Stopping server...");
        running = false;

        try {
            // Close the acceptor to interrupt any pending accept operations
            if (acceptor) {
                boost::system::error_code ec;
                acceptor->close(ec);
            }

            // Stop the io_context
            ioc.stop();

            // Wait for the server thread to finish
            if (serverThread.joinable()) {
                serverThread.join();
            }

            Logger::info("CaptchaServer", "Server stopped");
        } catch (const std::exception& e) {
            Logger::error("CaptchaServer", "Error stopping server: " + std::string(e.what()));
        }
    }

    bool isRunning() const {
        return running;
    }

private:
    void handleConnection(std::shared_ptr<tcp::socket> socket) {
        try {
            // Read the HTTP request
            boost::beast::flat_buffer buffer;
            http::request<http::string_body> request;
            boost::beast::http::read(*socket, buffer, request);

            // Log request details
            Logger::info("CaptchaServer", "Received " + std::string(request.method_string()) + " request to " + std::string(request.target()));

            // Prepare common response headers
            auto makeResponse = [](auto&& body, unsigned version) {
                http::response<http::string_body> response{
                    std::piecewise_construct,
                    std::make_tuple(std::move(body)),
                    std::make_tuple(http::status::ok, version)
                };
                response.set(http::field::server, "CaptchaServer");
                response.set(http::field::content_type, "text/plain");
                response.set(http::field::access_control_allow_origin, "*");
                response.set(http::field::access_control_allow_methods, "POST, OPTIONS");
                response.set(http::field::access_control_allow_headers, "Content-Type");
                return response;
            };

            // Handle CORS preflight request
            if (request.method() == http::verb::options) {
                auto response = makeResponse("", request.version());
                response.result(http::status::no_content);
                http::write(*socket, response);
                return;
            }

            // Handle POST request with token
            if (request.method() == http::verb::post) {
                std::string token = request.body();
                Logger::info("CaptchaServer", "Received token submission");

                if (!token.empty()) {
                    if (callback) {
                        callback(token);
                        auto response = makeResponse("Token received", request.version());
                        http::write(*socket, response);
                    } else {
                        auto response = makeResponse("No callback registered", request.version());
                        response.result(http::status::internal_server_error);
                        http::write(*socket, response);
                    }
                } else {
                    auto response = makeResponse("Empty token", request.version());
                    response.result(http::status::bad_request);
                    http::write(*socket, response);
                }
                return;
            }

            // Handle unknown requests
            auto response = makeResponse("Not Found", request.version());
            response.result(http::status::not_found);
            http::write(*socket, response);

        } catch (const std::exception& e) {
            Logger::error("CaptchaServer", "Error handling connection: " + std::string(e.what()));
        }

        // Close the socket
        boost::system::error_code ec;
        socket->shutdown(tcp::socket::shutdown_both, ec);
        socket->close(ec);
    }

    uint16_t port;
    CaptchaCallback callback;
    net::io_context ioc;
    std::unique_ptr<tcp::acceptor> acceptor;
    std::thread serverThread;
};

CaptchaServer::CaptchaServer(uint16_t port, CaptchaCallback callback)
    : impl(std::make_unique<Impl>(port, callback))
{
}

CaptchaServer::~CaptchaServer() = default;

void CaptchaServer::start() {
    impl->start();
}

void CaptchaServer::stop() {
    impl->stop();
}

bool CaptchaServer::isRunning() const {
    return impl->isRunning();
}

} // namespace owop 