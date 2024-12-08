#pragma once

// Include websocketpp headers outside of any namespace
#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>
#include <websocketpp/transport/asio/security/tls.hpp>

// Define websocket types outside of any namespace
using WebSocketClient = websocketpp::client<websocketpp::config::asio_tls_client>;
using WebSocketConnection = websocketpp::connection_hdl; 