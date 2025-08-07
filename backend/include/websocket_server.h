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

// Database initialization function
void init_websocket_database(const std::string& connection_string);

// JWT verification function
bool verify_jwt_token(const std::string& token, std::string& user_id, std::string& username);

// Production utility functions
std::string base64_decode(const std::string& encoded);
std::vector<std::pair<std::string, std::string>> get_real_users_from_main_db();
bool validate_user_exists_in_main_db(const std::string& user_id);

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
    
    // MOVE THIS FROM PRIVATE TO PUBLIC:
    void start_maintenance_tasks();    // <-- Add this line here
    
private:
    // Session handling
    void handle_session(boost::beast::tcp_stream stream, const std::string& client_endpoint);
    
    // Performance monitoring
    void cleanup_inactive_sessions();
    // void start_maintenance_tasks();  <-- Remove from here
};

} // namespace caffis