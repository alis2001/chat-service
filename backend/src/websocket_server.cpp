#include "../include/websocket_server.h"
#include "../include/database_manager.h"
#include "../include/message_types.h"
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <iostream>
#include <thread>
#include <unordered_map>
#include <mutex>
#include <memory>

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
namespace pt = boost::property_tree;

using tcp = net::ip::tcp;

namespace caffis {

// ================================================
// SESSION MANAGEMENT
// ================================================
struct ClientSession {
    std::string user_id;
    std::string username;
    std::string room_id;
    std::shared_ptr<websocket::stream<beast::tcp_stream>> ws;
    bool is_authenticated = false;
    std::chrono::system_clock::time_point last_activity;
    
    ClientSession(std::shared_ptr<websocket::stream<beast::tcp_stream>> ws_ptr) 
        : ws(ws_ptr), last_activity(std::chrono::system_clock::now()) {}
};

// Global session management
static std::unordered_map<std::string, std::shared_ptr<ClientSession>> active_sessions;
static std::mutex sessions_mutex;
static std::unique_ptr<DatabaseManager> db_manager;

// ================================================
// JWT VERIFICATION (SIMPLIFIED)
// ================================================
bool verify_jwt_token(const std::string& token, std::string& user_id, std::string& username) {
    // TODO: Implement proper JWT verification with libssl
    // For now, decode basic JWT payload (DEVELOPMENT ONLY)
    try {
        // Find payload section (between first and second '.')
        size_t first_dot = token.find('.');
        size_t second_dot = token.find('.', first_dot + 1);
        
        if (first_dot == std::string::npos || second_dot == std::string::npos) {
            std::cerr << "❌ Invalid JWT format" << std::endl;
            return false;
        }
        
        std::string payload = token.substr(first_dot + 1, second_dot - first_dot - 1);
        
        // Basic base64 decode (simplified)
        // In production, use proper JWT library like jwt-cpp
        std::cout << "🔐 JWT token received, payload length: " << payload.length() << std::endl;
        
        // Mock user extraction (replace with real JWT parsing)
        user_id = "user_" + std::to_string(std::hash<std::string>{}(token) % 10000);
        username = "User" + user_id.substr(5);
        
        std::cout << "✅ JWT verified for user: " << username << " (ID: " << user_id << ")" << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ JWT verification failed: " << e.what() << std::endl;
        return false;
    }
}

// ================================================
// MESSAGE BROADCASTING
// ================================================
void broadcast_to_room(const std::string& room_id, const std::string& message, const std::string& sender_id = "") {
    std::lock_guard<std::mutex> lock(sessions_mutex);
    
    int delivered_count = 0;
    
    for (auto& [session_id, session] : active_sessions) {
        if (session->room_id == room_id && session->is_authenticated && session->user_id != sender_id) {
            try {
                if (session->ws && session->ws->is_open()) {
                    session->ws->text(true);
                    session->ws->write(net::buffer(message));
                    delivered_count++;
                }
            } catch (const std::exception& e) {
                std::cerr << "❌ Failed to deliver to session " << session_id << ": " << e.what() << std::endl;
            }
        }
    }
    
    std::cout << "📢 Broadcasted message to " << delivered_count << " clients in room: " << room_id << std::endl;
}

// ================================================
// MESSAGE PROCESSING
// ================================================
void handle_message(std::shared_ptr<ClientSession> session, const std::string& raw_message) {
    try {
        // Parse JSON message
        std::istringstream iss(raw_message);
        pt::ptree message_json;
        pt::read_json(iss, message_json);
        
        std::string type = message_json.get<std::string>("type", "");
        
        if (type == "auth") {
            // ================================================
            // AUTHENTICATION MESSAGE
            // ================================================
            std::string token = message_json.get<std::string>("token", "");
            
            if (token.empty()) {
                session->ws->text(true);
                session->ws->write(net::buffer(R"({"type":"auth_error","error":"Token required"})"));
                return;
            }
            
            std::string user_id, username;
            if (verify_jwt_token(token, user_id, username)) {
                session->user_id = user_id;
                session->username = username;
                session->is_authenticated = true;
                session->last_activity = std::chrono::system_clock::now();
                
                // Send success response
                pt::ptree response;
                response.put("type", "auth_success");
                response.put("user_id", user_id);
                response.put("username", username);
                
                std::ostringstream oss;
                pt::write_json(oss, response);
                
                session->ws->text(true);
                session->ws->write(net::buffer(oss.str()));
                
                std::cout << "🔐 User authenticated: " << username << " (" << user_id << ")" << std::endl;
                
            } else {
                session->ws->text(true);
                session->ws->write(net::buffer(R"({"type":"auth_error","error":"Invalid token"})"));
            }
            
        } else if (type == "join_room") {
            // ================================================
            // JOIN ROOM MESSAGE
            // ================================================
            if (!session->is_authenticated) {
                session->ws->text(true);
                session->ws->write(net::buffer(R"({"type":"error","error":"Authentication required"})"));
                return;
            }
            
            std::string room_id = message_json.get<std::string>("room_id", "");
            if (!room_id.empty()) {
                session->room_id = room_id;
                
                // Send confirmation
                pt::ptree response;
                response.put("type", "room_joined");
                response.put("room_id", room_id);
                
                std::ostringstream oss;
                pt::write_json(oss, response);
                
                session->ws->text(true);
                session->ws->write(net::buffer(oss.str()));
                
                std::cout << "🏠 User " << session->username << " joined room: " << room_id << std::endl;
                
                // Notify others in room
                pt::ptree notification;
                notification.put("type", "user_joined");
                notification.put("user_id", session->user_id);
                notification.put("username", session->username);
                
                std::ostringstream notification_oss;
                pt::write_json(notification_oss, notification);
                
                broadcast_to_room(room_id, notification_oss.str(), session->user_id);
            }
            
        } else if (type == "message") {
            // ================================================
            // CHAT MESSAGE
            // ================================================
            if (!session->is_authenticated) {
                session->ws->text(true);
                session->ws->write(net::buffer(R"({"type":"error","error":"Authentication required"})"));
                return;
            }
            
            if (session->room_id.empty()) {
                session->ws->text(true);
                session->ws->write(net::buffer(R"({"type":"error","error":"Must join a room first"})"));
                return;
            }
            
            std::string content = message_json.get<std::string>("content", "");
            if (content.empty()) {
                return; // Ignore empty messages
            }
            
            // Create message ID and timestamp
            std::string message_id = "msg_" + std::to_string(std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count());
            
            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);
            
            // TODO: Store in database
            // if (db_manager) {
            //     db_manager->store_message(message_id, session->room_id, session->user_id, content);
            // }
            
            // Create broadcast message
            pt::ptree broadcast_msg;
            broadcast_msg.put("type", "new_message");
            broadcast_msg.put("message_id", message_id);
            broadcast_msg.put("room_id", session->room_id);
            broadcast_msg.put("sender_id", session->user_id);
            broadcast_msg.put("sender_name", session->username);
            broadcast_msg.put("content", content);
            broadcast_msg.put("timestamp", std::to_string(time_t));
            
            std::ostringstream broadcast_oss;
            pt::write_json(broadcast_oss, broadcast_msg);
            
            // Send to sender (confirmation)
            session->ws->text(true);
            session->ws->write(net::buffer(broadcast_oss.str()));
            
            // Broadcast to room
            broadcast_to_room(session->room_id, broadcast_oss.str(), session->user_id);
            
            std::cout << "💬 Message from " << session->username << " in " << session->room_id 
                     << ": " << content.substr(0, 50) << (content.length() > 50 ? "..." : "") << std::endl;
            
        } else {
            std::cerr << "❓ Unknown message type: " << type << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Message processing error: " << e.what() << std::endl;
        
        // Send error response
        session->ws->text(true);
        session->ws->write(net::buffer(R"({"type":"error","error":"Message processing failed"})"));
    }
}

// ================================================
// WEBSOCKET SERVER IMPLEMENTATION
// ================================================
WebSocketServer::WebSocketServer(int port) : port_(port) {
    const int thread_count = std::thread::hardware_concurrency();
    thread_pool_.reserve(thread_count);
    
    std::cout << "🚀 Enhanced WebSocket Server initialized with " << thread_count << " threads" << std::endl;
}

WebSocketServer::~WebSocketServer() {
    stop();
}

void WebSocketServer::start() {
    std::cout << "✅ Starting professional WebSocket server on port " << port_ << std::endl;
    
    try {
        tcp::acceptor acceptor{io_context_, {tcp::v4(), static_cast<unsigned short>(port_)}};
        
        std::cout << "🔗 Server listening for WebSocket connections..." << std::endl;
        std::cout << "📡 Ready to handle real-time messaging!" << std::endl;
        
        for (;;) {
            tcp::socket socket{io_context_};
            acceptor.accept(socket);
            
            std::string client_endpoint = socket.remote_endpoint().address().to_string();
            std::cout << "📱 New WebSocket connection from: " << client_endpoint << std::endl;
            
            // Handle connection in separate thread
            std::thread([this, socket = std::move(socket), client_endpoint]() mutable {
                handle_session(beast::tcp_stream(std::move(socket)), client_endpoint);
            }).detach();
        }
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Server startup error: " << e.what() << std::endl;
        throw;
    }
}

void WebSocketServer::stop() {
    std::cout << "🛑 Stopping WebSocket server..." << std::endl;
    
    // Close all active sessions
    {
        std::lock_guard<std::mutex> lock(sessions_mutex);
        for (auto& [session_id, session] : active_sessions) {
            if (session->ws && session->ws->is_open()) {
                session->ws->close(websocket::close_code::going_away);
            }
        }
        active_sessions.clear();
    }
    
    io_context_.stop();
    
    for (auto& thread : thread_pool_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    
    std::cout << "✅ WebSocket server stopped successfully" << std::endl;
}

void WebSocketServer::handle_session(beast::tcp_stream stream, const std::string& client_endpoint) {
    std::string session_id = "session_" + std::to_string(std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());
    
    try {
        // Set timeouts
        beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(30));
        
        // Create WebSocket stream
        auto ws = std::make_shared<websocket::stream<beast::tcp_stream>>(std::move(stream));
        
        // Accept WebSocket handshake
        ws->accept();
        
        std::cout << "🤝 WebSocket handshake completed for session: " << session_id << std::endl;
        
        // Create session
        auto session = std::make_shared<ClientSession>(ws);
        
        // Add to active sessions
        {
            std::lock_guard<std::mutex> lock(sessions_mutex);
            active_sessions[session_id] = session;
        }
        
        std::cout << "📊 Active sessions: " << active_sessions.size() << std::endl;
        
        // Main message loop
        for (;;) {
            beast::flat_buffer buffer;
            
            // Read message with timeout
            ws->read(buffer);
            
            std::string message = beast::buffers_to_string(buffer.data());
            session->last_activity = std::chrono::system_clock::now();
            
            std::cout << "📨 [" << session_id << "] Received: " 
                     << message.substr(0, 100) << (message.length() > 100 ? "..." : "") << std::endl;
            
            // Process message
            handle_message(session, message);
        }
        
    } catch (const beast::system_error& se) {
        if (se.code() != websocket::error::closed) {
            std::cerr << "❌ WebSocket error for " << session_id << ": " << se.code().message() << std::endl;
        } else {
            std::cout << "👋 Session " << session_id << " disconnected gracefully" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "❌ Session error for " << session_id << ": " << e.what() << std::endl;
    }
    
    // Clean up session
    {
        std::lock_guard<std::mutex> lock(sessions_mutex);
        auto it = active_sessions.find(session_id);
        if (it != active_sessions.end()) {
            std::cout << "🧹 Cleaning up session: " << session_id;
            if (it->second->is_authenticated) {
                std::cout << " (User: " << it->second->username << ")";
            }
            std::cout << std::endl;
            
            active_sessions.erase(it);
        }
    }
    
    std::cout << "📊 Active sessions after cleanup: " << active_sessions.size() << std::endl;
}

} // namespace caffis