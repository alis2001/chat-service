#include "../include/websocket_server.h"
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <iostream>
#include <thread>

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;

using tcp = net::ip::tcp;

namespace caffis {

WebSocketServer::WebSocketServer(int port) : port_(port) {
    // Initialize thread pool
    const int thread_count = std::thread::hardware_concurrency();
    thread_pool_.reserve(thread_count);
}

WebSocketServer::~WebSocketServer() {
    stop();
}

void WebSocketServer::start() {
    std::cout << "✅ WebSocket server starting on port " << port_ << std::endl;
    
    try {
        // Create acceptor
        tcp::acceptor acceptor{io_context_, {tcp::v4(), static_cast<unsigned short>(port_)}};
        
        std::cout << "🔗 Server listening for connections..." << std::endl;
        
        // Start accepting connections
        for (;;) {
            tcp::socket socket{io_context_};
            acceptor.accept(socket);
            
            std::cout << "📱 New connection from: " << socket.remote_endpoint() << std::endl;
            
            // Handle connection in separate thread
            std::thread([this, socket = std::move(socket)]() mutable {
                handle_session(beast::tcp_stream(std::move(socket)));
            }).detach();
        }
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Server error: " << e.what() << std::endl;
    }
}

void WebSocketServer::stop() {
    std::cout << "🛑 Stopping WebSocket server..." << std::endl;
    io_context_.stop();
    
    // Wait for all threads to finish
    for (auto& thread : thread_pool_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    
    std::cout << "✅ Server stopped successfully" << std::endl;
}

void WebSocketServer::handle_session(beast::tcp_stream stream) {
    try {
        // Set timeout
        beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(30));
        
        // Create WebSocket
        websocket::stream<beast::tcp_stream> ws{std::move(stream)};
        
        // Accept WebSocket handshake
        ws.accept();
        
        std::cout << "🤝 WebSocket handshake completed" << std::endl;
        
        // Main message loop
        for (;;) {
            beast::flat_buffer buffer;
            
            // Read message
            ws.read(buffer);
            
            // Echo message back (for now)
            std::string message = beast::buffers_to_string(buffer.data());
            std::cout << "📨 Received: " << message << std::endl;
            
            // Send response
            ws.text(ws.got_text());
            ws.write(net::buffer("Echo: " + message));
        }
        
    } catch (const beast::system_error& se) {
        if (se.code() != websocket::error::closed) {
            std::cerr << "❌ WebSocket error: " << se.code().message() << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "❌ Session error: " << e.what() << std::endl;
    }
}

} // namespace caffis
