#include "../include/websocket_server.h"
#include "../include/config.h"
#include <iostream>
#include <csignal>
#include <memory>

std::unique_ptr<caffis::WebSocketServer> server;

void signal_handler(int signal) {
    std::cout << "\nReceived signal " << signal << ". Shutting down gracefully..." << std::endl;
    if (server) {
        server->stop();
    }
    exit(0);
}

int main() {
    std::cout << "🚀 Starting Caffis Chat Service..." << std::endl;
    
    // Register signal handlers
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
    
    try {
        // Load configuration
        caffis::config::ServerConfig config;
        config.port = 5004;
        
        std::cout << "📡 Starting WebSocket server on port " << config.port << std::endl;
        
        // Create and start server
        server = std::make_unique<caffis::WebSocketServer>(config.port);
        server->start();
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
