#include "../include/websocket_server.h"
#include "../include/database_manager.h"
#include "../include/message_types.h"
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/algorithm/string.hpp>
#include <iostream>
#include <thread>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <sstream>
#include <algorithm>
#include <pqxx/pqxx>

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
namespace pt = boost::property_tree;

using tcp = net::ip::tcp;

namespace caffis {

// ================================================
// PRODUCTION DATABASE CONNECTION
// ================================================
static std::unique_ptr<pqxx::connection> main_app_connection;
static std::mutex main_db_mutex;

void init_main_app_connection() {
    try {
        // Default connection string for main app database
        std::string main_db_url = "postgresql://caffis_user:admin5026@caffis-db:5432/caffis_db";
        
        // Override with environment variable if available
        const char* main_db_env = std::getenv("MAIN_DATABASE_URL");
        if (main_db_env) {
            main_db_url = std::string(main_db_env);
        }
        
        main_app_connection = std::make_unique<pqxx::connection>(main_db_url);
        std::cout << "✅ Main app database connection established" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "⚠️ Main app database connection failed: " << e.what() << std::endl;
    }
}

// ================================================
// SESSION MANAGEMENT
// ================================================
struct ClientSession {
    std::string user_id;
    std::string username;
    std::string display_name;
    std::string email;
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
// DATABASE INITIALIZATION FOR WEBSOCKET
// ================================================
void init_websocket_database(const std::string& connection_string) {
    try {
        std::cout << "🗄️ Initializing WebSocket database manager..." << std::endl;
        db_manager = std::make_unique<DatabaseManager>(connection_string);
        if (db_manager->connect()) {
            std::cout << "✅ WebSocket database manager connected successfully" << std::endl;
        } else {
            std::cerr << "⚠️ WebSocket database connection failed - continuing without DB" << std::endl;
            db_manager.reset();
        }
        
        // Initialize main app database connection
        init_main_app_connection();
        
    } catch (const std::exception& e) {
        std::cerr << "⚠️ WebSocket database error: " << e.what() << std::endl;
        db_manager.reset();
    }
}

// ================================================
// PRODUCTION UTILITY FUNCTIONS
// ================================================
std::string base64_decode(const std::string& encoded) {
    static const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    
    std::string decoded;
    std::vector<int> T(256, -1);
    
    for (int i = 0; i < 64; i++) T[chars[i]] = i;
    
    int val = 0, valb = -8;
    for (unsigned char c : encoded) {
        if (T[c] == -1) break;
        val = (val << 6) + T[c];
        valb += 6;
        if (valb >= 0) {
            decoded.push_back(char((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    
    return decoded;
}

// JWT Base64 URL decode function
std::string base64url_decode(const std::string& input) {
    std::string base64 = input;
    
    // Replace URL-safe characters
    std::replace(base64.begin(), base64.end(), '-', '+');
    std::replace(base64.begin(), base64.end(), '_', '/');
    
    // Add padding if needed
    while (base64.length() % 4 != 0) {
        base64 += '=';
    }
    
    // Use existing base64_decode function
    return base64_decode(base64);
}

std::vector<std::pair<std::string, std::string>> get_real_users_from_main_db() {
    std::vector<std::pair<std::string, std::string>> users;
    
    try {
        if (!main_app_connection) {
            std::cerr << "❌ Main app database not connected" << std::endl;
            return users;
        }
        
        std::lock_guard<std::mutex> lock(main_db_mutex);
        pqxx::work txn(*main_app_connection);
        
        pqxx::result result = txn.exec("SELECT id, username FROM \"User\" WHERE username IS NOT NULL ORDER BY \"createdAt\" DESC LIMIT 20");
        
        for (auto row : result) {
            std::string id = row[0].c_str();
            std::string username = row[1].c_str();
            users.push_back({id, username});
        }
        
        txn.commit();
        std::cout << "✅ Fetched " << users.size() << " real users from main database" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Failed to fetch real users: " << e.what() << std::endl;
    }
    
    return users;
}

bool validate_user_exists_in_main_db(const std::string& user_id) {
    try {
        if (!main_app_connection) {
            return false;
        }
        
        std::lock_guard<std::mutex> lock(main_db_mutex);
        pqxx::work txn(*main_app_connection);
        
        pqxx::result result = txn.exec_params("SELECT COUNT(*) FROM \"User\" WHERE id = $1", user_id);
        txn.commit();
        
        return result[0][0].as<int>() > 0;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ User validation error: " << e.what() << std::endl;
        return false;
    }
}

struct UserDetails {
    std::string id;
    std::string username;
    std::string firstName;
    std::string lastName;
    std::string email;
    std::string profilePic;
    std::string bio;
    bool found = false;
};

UserDetails get_user_details_from_main_db(const std::string& user_id) {
    UserDetails details;
    
    try {
        if (!main_app_connection) {
            std::cerr << "❌ Main app database not connected" << std::endl;
            return details;
        }
        
        std::lock_guard<std::mutex> lock(main_db_mutex);
        pqxx::work txn(*main_app_connection);
        
        // First, let's check if user exists at all
        pqxx::result exists_result = txn.exec_params(
            "SELECT COUNT(*) FROM \"User\" WHERE id = $1", 
            user_id
        );
        
        int user_count = exists_result[0][0].as<int>();
        std::cout << "🔍 User existence check: " << user_count << " users found with ID " << user_id << std::endl;
        
        if (user_count == 0) {
            std::cout << "❌ No user exists with ID: " << user_id << std::endl;
            txn.commit();
            return details;
        }
        
        // User exists, let's get their details
        // FIXED: Access columns by index to avoid name resolution issues
        pqxx::result result = txn.exec_params(
            "SELECT id, username, \"firstName\", \"lastName\", email, \"profilePic\", bio "
            "FROM \"User\" WHERE id = $1", 
            user_id
        );
        
        if (!result.empty()) {
            auto row = result[0];
            
            // Access by index to avoid column name issues
            details.id = row[0].c_str();                    // id (index 0)
            details.username = row[1].is_null() ? "" : row[1].c_str();  // username (index 1)
            details.firstName = row[2].is_null() ? "" : row[2].c_str(); // firstName (index 2)
            details.lastName = row[3].is_null() ? "" : row[3].c_str();  // lastName (index 3)
            details.email = row[4].is_null() ? "" : row[4].c_str();     // email (index 4)
            details.profilePic = row[5].is_null() ? "" : row[5].c_str(); // profilePic (index 5)
            details.bio = row[6].is_null() ? "" : row[6].c_str();       // bio (index 6)
            details.found = true;
            
            std::cout << "✅ Found user details: " << details.firstName << " " << details.lastName 
                      << " (username: " << details.username << ", email: " << details.email << ")" << std::endl;
        } else {
            std::cout << "❌ Query returned no results for user ID: " << user_id << std::endl;
        }
        
        txn.commit();
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Failed to get user details: " << e.what() << std::endl;
    }
    
    return details;
}

// ================================================
// CORRECTED JWT VERIFICATION FUNCTION
// ================================================
bool verify_jwt_token(const std::string& token, std::string& user_id, std::string& username) {
    try {
        std::cout << "🔐 Real JWT verification starting..." << std::endl;
        
        // Split JWT into parts
        std::vector<std::string> parts;
        boost::split(parts, token, boost::is_any_of("."));
        
        if (parts.size() != 3) {
            std::cerr << "❌ Invalid JWT format - expected 3 parts, got " << parts.size() << std::endl;
            return false;
        }
        
        // Decode payload (second part)
        std::string payload_json;
        try {
            payload_json = base64url_decode(parts[1]);
            std::cout << "🔍 Decoded JWT payload: " << payload_json << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "❌ Failed to decode JWT payload: " << e.what() << std::endl;
            return false;
        }
        
        // Parse JSON payload
        boost::property_tree::ptree payload_tree;
        std::istringstream payload_stream(payload_json);
        
        try {
            boost::property_tree::read_json(payload_stream, payload_tree);
        } catch (const std::exception& e) {
            std::cerr << "❌ Failed to parse JWT payload JSON: " << e.what() << std::endl;
            return false;
        }
        
        // Extract user ID from payload
        std::string jwt_user_id = payload_tree.get<std::string>("id", "");
        if (jwt_user_id.empty()) {
            std::cerr << "❌ No user ID found in JWT payload" << std::endl;
            return false;
        }
        
        std::cout << "✅ Extracted user ID from JWT: " << jwt_user_id << std::endl;
        
        // Fetch real user details from main database
        UserDetails user_details = get_user_details_from_main_db(jwt_user_id);
        
        if (!user_details.found) {
            std::cerr << "❌ User not found in main database: " << jwt_user_id << std::endl;
            return false;
        }
        
        // Set return values with real data
        user_id = user_details.id;
        username = !user_details.username.empty() ? user_details.username : user_details.firstName;
        
        std::cout << "✅ JWT verified - Real user: " << username << " (ID: " << user_id.substr(0, 8) << "...)" << std::endl;
        
        // Auto-sync real user to chat database
        if (db_manager) {
            try {
                std::cout << "🔄 Auto-syncing REAL user to chat database..." << std::endl;
                
                std::string display_name = user_details.firstName;
                if (!user_details.lastName.empty()) {
                    display_name += " " + user_details.lastName;
                }
                
                std::string email = user_details.email.empty() ? (username + "@caffis.com") : user_details.email;
                
                bool sync_success = db_manager->sync_user(
                    user_id, 
                    username, 
                    display_name, 
                    email, 
                    user_details.profilePic
                );
                
                if (sync_success) {
                    std::cout << "✅ REAL user auto-synced: " << username << " (" << display_name << ")" << std::endl;
                    db_manager->update_user_status(user_id, true);
                    std::cout << "🟢 User status: online" << std::endl;
                }
                
            } catch (const std::exception& e) {
                std::cerr << "⚠️ Auto-sync error: " << e.what() << std::endl;
            }
        }
        
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
    int total_in_room = 0;
    
    std::cout << "🔍 Broadcasting to room: " << room_id << " (excluding sender: " << sender_id.substr(0, 8) << "...)" << std::endl;
    
    for (auto& [session_id, session] : active_sessions) {
        if (session->room_id == room_id && session->is_authenticated) {
            total_in_room++;
            
            if (session->user_id != sender_id) {
                try {
                    if (session->ws && session->ws->is_open()) {
                        session->ws->text(true);
                        session->ws->write(net::buffer(message));
                        delivered_count++;
                        std::cout << "   ✅ Delivered to " << session->username << std::endl;
                    }
                } catch (const std::exception& e) {
                    std::cerr << "   ❌ Failed to deliver to " << session->username << ": " << e.what() << std::endl;
                }
            }
        }
    }
    
    std::cout << "📢 Broadcast complete: " << delivered_count << " delivered out of " << total_in_room << " users" << std::endl;
}

// ================================================
// MESSAGE PROCESSING
// ================================================
void handle_message(std::shared_ptr<ClientSession> session, const std::string& raw_message) {
    try {
        std::istringstream iss(raw_message);
        pt::ptree message_json;
        pt::read_json(iss, message_json);
        
        std::string type = message_json.get<std::string>("type", "");
        
        if (type == "auth") {
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
                
                // Get full user details
                UserDetails user_details = get_user_details_from_main_db(user_id);
                if (user_details.found) {
                    session->display_name = user_details.firstName + " " + user_details.lastName;
                    session->email = user_details.email;
                }
                
                // Send success response
                pt::ptree response;
                response.put("type", "auth_success");
                response.put("user_id", user_id);
                response.put("username", username);
                response.put("display_name", session->display_name);
                
                std::ostringstream response_oss;
                pt::write_json(response_oss, response);
                
                session->ws->text(true);
                session->ws->write(net::buffer(response_oss.str()));
                
                std::cout << "🔐 User authenticated: " << username << std::endl;
                
                // ================================================
                // AUTO-CREATE DEFAULT ROOM AND AUTO-JOIN USER
                // ================================================
                if (db_manager) {
                    try {
                        // Ensure user is in default room (creates room if needed)
                        bool auto_room_success = db_manager->ensure_user_in_default_room(session->user_id, session->username);
                        
                        if (auto_room_success) {
                            std::cout << "✅ User " << session->username << " auto-added to default room" << std::endl;
                            
                            // Send available rooms to user
                            std::vector<ChatRoom> user_rooms = db_manager->get_user_rooms(session->user_id);
                            
                            pt::ptree rooms_response;
                            rooms_response.put("type", "rooms_list");
                            
                            pt::ptree rooms_array;
                            for (const auto& room : user_rooms) {
                                pt::ptree room_obj;
                                room_obj.put("id", room.id);
                                room_obj.put("name", room.name);
                                room_obj.put("type", room.type);
                                room_obj.put("isOnline", true);
                                rooms_array.push_back(std::make_pair("", room_obj));
                            }
                            
                            rooms_response.add_child("rooms", rooms_array);
                            
                            std::ostringstream rooms_oss;
                            pt::write_json(rooms_oss, rooms_response);
                            
                            session->ws->text(true);
                            session->ws->write(net::buffer(rooms_oss.str()));
                            
                            std::cout << "📋 Sent " << user_rooms.size() << " available rooms to " << session->username << std::endl;
                        }
                        
                    } catch (const std::exception& e) {
                        std::cerr << "❌ Auto-room setup failed: " << e.what() << std::endl;
                    }
                }
                
            } else {
                session->ws->text(true);
                session->ws->write(net::buffer(R"({"type":"auth_error","error":"Invalid token"})"));
            }
            
        } else if (type == "message") {
            if (!session->is_authenticated) {
                session->ws->text(true);
                session->ws->write(net::buffer(R"({"type":"error","error":"Authentication required"})"));
                return;
            }
            
            std::string roomId = message_json.get<std::string>("roomId", "");
            std::string content = message_json.get<std::string>("content", "");
            std::string timestamp = message_json.get<std::string>("timestamp", "");
            
            if (roomId.empty() || content.empty()) {
                session->ws->text(true);
                session->ws->write(net::buffer(R"({"type":"error","error":"Room ID and content required"})"));
                return;
            }
            
            // Generate message ID and timestamp
            auto now = std::chrono::system_clock::now();
            auto epoch = now.time_since_epoch();
            auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(epoch).count();
            std::string message_id = "msg_" + std::to_string(millis);
            
            // Create message for frontend (new_message format)
            pt::ptree msg_response;
            msg_response.put("type", "new_message");
            msg_response.put("message_id", message_id);
            msg_response.put("room_id", roomId);  
            msg_response.put("sender_id", session->user_id);
            msg_response.put("sender_name", session->display_name.empty() ? session->username : session->display_name);
            msg_response.put("content", content);
            msg_response.put("timestamp", std::to_string(millis));
            msg_response.put("message_type", "text");
            
            std::ostringstream msg_oss;
            pt::write_json(msg_oss, msg_response);
            
            std::cout << "💬 Message from " << session->username << ": " << content << std::endl;
            
            // Broadcast to ALL users in room (including sender for confirmation)
            broadcast_to_room(roomId, msg_oss.str(), "");
            
            // Save to database
            if (db_manager) {
                try {
                    Message msg;
                    msg.id = message_id;
                    msg.room_id = roomId;
                    msg.sender_id = session->user_id;
                    msg.content = content;
                    msg.type = MessageType::TEXT;
                    msg.is_edited = false;
                    msg.is_deleted = false;
                    
                    std::string saved_id = db_manager->save_message(msg);
                    if (!saved_id.empty()) {
                        std::cout << "💾 Message saved: " << saved_id << std::endl;
                    }
                } catch (const std::exception& e) {
                    std::cerr << "❌ Database save failed: " << e.what() << std::endl;
                }
            }
            
        } else if (type == "join_room") {
            if (!session->is_authenticated) {
                session->ws->text(true);
                session->ws->write(net::buffer(R"({"type":"error","error":"Authentication required"})"));
                return;
            }
            
            std::string room_id = message_json.get<std::string>("room_id", "");
            
            if (room_id.empty()) {
                session->ws->text(true);
                session->ws->write(net::buffer(R"({"type":"error","error":"Room ID required"})"));
                return;
            }
            
            std::cout << "🏠 User " << session->username << " joining room: " << room_id << std::endl;
            
            // Check if user can join this room (is participant)
            if (db_manager) {
                try {
                    bool can_join = db_manager->can_user_join_room(session->user_id, room_id);
                    
                    if (!can_join) {
                        session->ws->text(true);
                        session->ws->write(net::buffer(R"({"type":"error","error":"Access denied to room"})"));
                        return;
                    }
                    
                    // Set user's current room
                    session->room_id = room_id;
                    
                    // Add user as participant if not already
                    db_manager->add_participant(room_id, session->user_id, "member");
                    
                    // Send success response FIRST
                    pt::ptree join_response;
                    join_response.put("type", "room_joined");
                    join_response.put("room_id", room_id);
                    join_response.put("message", "Successfully joined room");

                    std::ostringstream join_oss;
                    pt::write_json(join_oss, join_response);

                    session->ws->text(true);
                    session->ws->write(net::buffer(join_oss.str()));

                    std::cout << "✅ User " << session->username << " joined room: " << room_id << std::endl;

                    // Load and send message history (FIXED - separate transaction)
                    try {
                        std::vector<Message> messages = db_manager->get_room_messages(room_id, 20);
                        
                        // Send messages in chronological order (oldest first)  
                        std::reverse(messages.begin(), messages.end());
                        
                        for (const auto& msg : messages) {
                            // Get sender details
                            std::string sender_username, sender_display_name;
                            db_manager->get_user(msg.sender_id, sender_username, sender_display_name);
                            
                            pt::ptree history_msg;
                            history_msg.put("type", "new_message");
                            history_msg.put("message_id", msg.id);
                            history_msg.put("room_id", msg.room_id);
                            history_msg.put("sender_id", msg.sender_id);
                            history_msg.put("sender_name", sender_display_name.empty() ? sender_username : sender_display_name);
                            history_msg.put("content", msg.content);
                            
                            // Convert timestamp
                            auto duration = msg.timestamp.time_since_epoch();
                            auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
                            history_msg.put("timestamp", std::to_string(millis));
                            history_msg.put("message_type", "text");
                            
                            std::ostringstream history_oss;
                            pt::write_json(history_oss, history_msg);
                            
                            session->ws->text(true);
                            session->ws->write(net::buffer(history_oss.str()));
                            
                            // Small delay for message ordering
                            std::this_thread::sleep_for(std::chrono::milliseconds(5));
                        }
                        
                        if (messages.size() > 0) {
                            std::cout << "📜 Sent " << messages.size() << " historical messages to " << session->username << std::endl;
                        }
                    } catch (const std::exception& e) {
                        std::cerr << "❌ Message history error: " << e.what() << std::endl;
                    }
                    
                } catch (const std::exception& e) {
                    std::cerr << "❌ Join room error: " << e.what() << std::endl;
                    session->ws->text(true);
                    session->ws->write(net::buffer(R"({"type":"error","error":"Failed to join room"})"));
                }
            } else {
                session->ws->text(true);
                session->ws->write(net::buffer(R"({"type":"error","error":"Database not available"})"));
            }
            
        } else {
            std::cerr << "❓ Unknown message type: " << type << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Message processing error: " << e.what() << std::endl;
        
        pt::ptree error_response;
        error_response.put("type", "error");
        error_response.put("error", "Message processing failed");
        
        std::ostringstream error_oss;
        pt::write_json(error_oss, error_response);
        
        try {
            session->ws->text(true);
            session->ws->write(net::buffer(error_oss.str()));
        } catch (const std::exception& send_error) {
            std::cerr << "❌ Failed to send error response" << std::endl;
        }
    }
}

// ================================================
// WEBSOCKET SERVER IMPLEMENTATION
// ================================================
WebSocketServer::WebSocketServer(int port) : port_(port) {
    const int thread_count = std::thread::hardware_concurrency();
    thread_pool_.reserve(thread_count);
    
    std::cout << "🚀 Production WebSocket Server initialized with " << thread_count << " threads" << std::endl;
}

WebSocketServer::~WebSocketServer() {
    stop();
}

void WebSocketServer::start() {
    std::cout << "✅ Starting production WebSocket server on port " << port_ << std::endl;
    
    try {
        tcp::acceptor acceptor{io_context_, {tcp::v4(), static_cast<unsigned short>(port_)}};
        
        std::cout << "🔗 Server ready for connections..." << std::endl;
        std::cout << "📡 Real-time messaging enabled!" << std::endl;
        
        for (;;) {
            tcp::socket socket{io_context_};
            acceptor.accept(socket);
            
            std::string client_endpoint = socket.remote_endpoint().address().to_string();
            std::cout << "📱 New connection from: " << client_endpoint << std::endl;
            
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
    
    {
        std::lock_guard<std::mutex> lock(sessions_mutex);
        for (auto& [session_id, session] : active_sessions) {
            if (session->ws && session->ws->is_open()) {
                if (db_manager && session->is_authenticated) {
                    db_manager->update_user_status(session->user_id, false);
                }
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
    
    std::cout << "✅ WebSocket server stopped" << std::endl;
}

void WebSocketServer::handle_session(beast::tcp_stream stream, const std::string& client_endpoint) {
    std::string session_id = "session_" + std::to_string(std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());
    
    try {
        beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(30));
        
        auto ws = std::make_shared<websocket::stream<beast::tcp_stream>>(std::move(stream));
        ws->accept();
        
        std::cout << "🤝 WebSocket handshake completed: " << session_id << std::endl;
        
        auto session = std::make_shared<ClientSession>(ws);
        
        {
            std::lock_guard<std::mutex> lock(sessions_mutex);
            active_sessions[session_id] = session;
        }
        
        std::cout << "📊 Active sessions: " << active_sessions.size() << std::endl;
        
        // Main message loop
        for (;;) {
            beast::flat_buffer buffer;
            ws->read(buffer);
            
            std::string message = beast::buffers_to_string(buffer.data());
            session->last_activity = std::chrono::system_clock::now();
            
            std::cout << "📨 [" << session_id << "] Received: " 
                     << message.substr(0, 100) << (message.length() > 100 ? "..." : "") << std::endl;
            
            handle_message(session, message);
        }
        
    } catch (const std::exception& e) {
        std::cout << "👋 Session disconnected: " << session_id << std::endl;
        if (active_sessions.count(session_id) && active_sessions[session_id]->is_authenticated) {
            std::cout << "🧹 Cleaning up: " << session_id << " (User: " << active_sessions[session_id]->username << ")";
            if (db_manager) {
                db_manager->update_user_status(active_sessions[session_id]->user_id, false);
            }
        } else {
            std::cout << "🧹 Cleaning up: " << session_id;
        }
        
        {
            std::lock_guard<std::mutex> lock(sessions_mutex);
            active_sessions.erase(session_id);
        }
        
        std::cout << std::endl << "📊 Active sessions: " << active_sessions.size() << std::endl;
    }
}

size_t WebSocketServer::get_active_connections() const {
    std::lock_guard<std::mutex> lock(sessions_mutex);
    return active_sessions.size();
}

std::string WebSocketServer::get_server_stats() const {
    std::lock_guard<std::mutex> lock(sessions_mutex);
    
    size_t authenticated_users = 0;
    for (const auto& [session_id, session] : active_sessions) {
        if (session->is_authenticated) {
            authenticated_users++;
        }
    }
    
    std::ostringstream stats;
    stats << "📊 Server Stats:\n";
    stats << "   • Total connections: " << active_sessions.size() << "\n";
    stats << "   • Authenticated users: " << authenticated_users << "\n";
    stats << "   • Server port: " << port_;
    
    return stats.str();
}

void WebSocketServer::cleanup_inactive_sessions() {
    std::lock_guard<std::mutex> lock(sessions_mutex);
    
    auto now = std::chrono::system_clock::now();
    auto it = active_sessions.begin();
    
    while (it != active_sessions.end()) {
        auto session = it->second;
        auto inactive_time = std::chrono::duration_cast<std::chrono::minutes>(now - session->last_activity);
        
        if (inactive_time.count() > 30) { // 30 minutes timeout
            std::cout << "🧹 Cleaning up inactive session: " << it->first << std::endl;
            
            if (db_manager && session->is_authenticated) {
                db_manager->update_user_status(session->user_id, false);
            }
            
            try {
                if (session->ws && session->ws->is_open()) {
                    session->ws->close(websocket::close_code::going_away);
                }
            } catch (const std::exception& e) {
                // Ignore cleanup errors
            }
            
            it = active_sessions.erase(it);
        } else {
            ++it;
        }
    }
}

void WebSocketServer::start_maintenance_tasks() {
    std::cout << "🔧 Maintenance tasks started" << std::endl;
    
    std::thread([this]() {
        while (true) {
            std::this_thread::sleep_for(std::chrono::minutes(5));
            cleanup_inactive_sessions();
            
            // FIXED: Use correct method name
            if (db_manager) {
                db_manager->cleanup_expired_typing_indicators();
            }
        }
    }).detach();
}

void WebSocketServer::set_database_manager(std::shared_ptr<DatabaseManager> db) {
    // This method can be used if needed
}

} // namespace caffis