#include "../include/websocket_server.h"
#include "../include/database_manager.h"
#include "../include/config.h"
#include <iostream>
#include <csignal>
#include <memory>
#include <cstdlib>
#include <string>

std::unique_ptr<caffis::WebSocketServer> server;
std::unique_ptr<caffis::DatabaseManager> database;

void signal_handler(int signal) {
    std::cout << "\nReceived signal " << signal << ". Shutting down gracefully..." << std::endl;
    
    if (server) {
        std::cout << "🔌 Stopping WebSocket server..." << std::endl;
        server->stop();
    }
    
    if (database) {
        std::cout << "🗄️ Disconnecting from database..." << std::endl;
        database->disconnect();
    }
    
    std::cout << "👋 Caffis Chat Service stopped" << std::endl;
    exit(0);
}

std::string get_env_var(const char* name, const std::string& default_value = "") {
    const char* value = std::getenv(name);
    return value ? std::string(value) : default_value;
}

int main() {
    std::cout << "🚀 Starting Caffis Chat Service..." << std::endl;
    
    // Register signal handlers for graceful shutdown
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
    
    try {
        // ================================================
        // 1. LOAD CONFIGURATION
        // ================================================
        std::cout << "⚙️ Loading configuration..." << std::endl;
        
        caffis::config::ServerConfig config;
        config.port = std::stoi(get_env_var("CHAT_PORT", "5004"));
        config.host = get_env_var("CHAT_HOST", "0.0.0.0");
        
        std::string db_url = get_env_var("DATABASE_URL");
        std::string redis_host = get_env_var("REDIS_HOST", "caffis-redis");
        int redis_port = std::stoi(get_env_var("REDIS_PORT", "6379"));
        
        std::cout << "✅ Configuration loaded:" << std::endl;
        std::cout << "   • Chat Port: " << config.port << std::endl;
        std::cout << "   • Chat Host: " << config.host << std::endl;
        std::cout << "   • Database: " << (db_url.empty() ? "❌ NOT SET" : "✅ Connected") << std::endl;
        std::cout << "   • Redis: " << redis_host << ":" << redis_port << std::endl;
        
        if (db_url.empty()) {
            std::cerr << "❌ DATABASE_URL environment variable not set!" << std::endl;
            std::cerr << "   Please set: DATABASE_URL=postgresql://chat_user:admin5026@chat-db:5432/chat_service" << std::endl;
            return 1;
        }
        
        // ================================================
        // 2. INITIALIZE DATABASE CONNECTION
        // ================================================
        std::cout << "\n🗄️ Initializing database connection..." << std::endl;
        
        database = std::make_unique<caffis::DatabaseManager>(db_url);
        
        if (!database->connect()) {
            std::cerr << "❌ Failed to connect to database!" << std::endl;
            std::cerr << "   Please ensure chat database is running and accessible" << std::endl;
            return 1;
        }
        
        // Test database connection
        if (!database->test_connection()) {
            std::cerr << "❌ Database health check failed!" << std::endl;
            return 1;
        }
        
        std::cout << database->get_database_stats() << std::endl;
        
        // ================================================
        // 3. INITIALIZE WEBSOCKET SERVER
        // ================================================
        std::cout << "\n📡 Initializing WebSocket server..." << std::endl;
        
        server = std::make_unique<caffis::WebSocketServer>(config.port);
        
        std::cout << "✅ WebSocket server initialized on port " << config.port << std::endl;
        
        // ================================================
        // 4. START SERVICES
        // ================================================
        std::cout << "\n🎯 Starting chat services..." << std::endl;
        std::cout << "===============================================" << std::endl;
        std::cout << "🚀 CAFFIS CHAT SERVICE READY!" << std::endl;
        std::cout << "===============================================" << std::endl;
        std::cout << "📡 WebSocket Server: ws://localhost:" << config.port << std::endl;
        std::cout << "🗄️ Database: Connected and healthy" << std::endl;
        std::cout << "🔴 Redis: " << redis_host << ":" << redis_port << std::endl;
        std::cout << "===============================================" << std::endl;
        
        // Start the server (this will block until shutdown)
        server->start();
        
    } catch (const std::exception& e) {
        std::cerr << "💥 FATAL ERROR: " << e.what() << std::endl;
        std::cerr << "❌ Caffis Chat Service failed to start!" << std::endl;
        return 1;
    }
    
    return 0;
}