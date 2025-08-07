#pragma once

#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <memory>
#include <thread>
#include <vector>
#include <string>

namespace caffis {

// Forward declarations
class DatabaseManager;

class WebSocketServer {
private:
    boost::asio::io_context io_context_;
    std::vector<std::thread> thread_pool_;
    int port_;

public:
    explicit WebSocketServer(int port);
    ~WebSocketServer();
    
    // Core server operations
    void start();
    void stop();
    
    // Database connection
    void set_database_manager(std::shared_ptr<DatabaseManager> db_manager);
    
    // Server statistics
    size_t get_active_connections() const;
    std::string get_server_stats() const;
    
private:
    // Session handling
    void handle_session(boost::beast::tcp_stream stream, const std::string& client_endpoint);
    
    // Performance monitoring
    void cleanup_inactive_sessions();
    void start_maintenance_tasks();
};

} // namespace caffis