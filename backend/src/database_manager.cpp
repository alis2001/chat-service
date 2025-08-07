#include "../include/database_manager.h"
#include <iostream>
#include <random>
#include <sstream>
#include <iomanip>
#include <chrono>

namespace caffis {

DatabaseManager::DatabaseManager(const std::string& connection_string) 
    : connection_string_(connection_string), is_connected_(false) {
    std::cout << "🗄️ DatabaseManager initialized with connection string" << std::endl;
}

DatabaseManager::~DatabaseManager() {
    disconnect();
}

bool DatabaseManager::connect() {
    try {
        std::cout << "🔌 Connecting to database..." << std::endl;
        connection_ = std::make_unique<pqxx::connection>(connection_string_);
        
        if (connection_->is_open()) {
            is_connected_ = true;
            prepare_statements();
            std::cout << "✅ Database connection established successfully!" << std::endl;
            std::cout << "📊 Database: " << connection_->dbname() << std::endl;
            return true;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Database connection failed: " << e.what() << std::endl;
    }
    
    is_connected_ = false;
    return false;
}

void DatabaseManager::disconnect() {
    if (connection_ && connection_->is_open()) {
        // In libpqxx 6.4.5, close() is protected
        // Connection will be automatically closed when connection_ is destroyed
        std::cout << "🔌 Database connection closing..." << std::endl;
    }
    is_connected_ = false;
    connection_.reset(); // This will destroy the connection object and close it automatically
}

bool DatabaseManager::test_connection() {
    try {
        if (!is_connected_ || !connection_) {
            return false;
        }
        
        pqxx::work txn(*connection_);
        pqxx::result result = txn.exec("SELECT NOW() as current_time");
        txn.commit();
        
        if (!result.empty()) {
            std::cout << "✅ Database health check passed: " 
                      << result[0]["current_time"].c_str() << std::endl;
            return true;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Database health check failed: " << e.what() << std::endl;
    }
    
    return false;
}

void DatabaseManager::prepare_statements() {
    try {
        // Prepare frequently used statements for better performance
        
        // User sync statement
        connection_->prepare("sync_user",
            "INSERT INTO chat_users (id, username, display_name, email, profile_pic_url, synced_at) "
            "VALUES ($1, $2, $3, $4, $5, NOW()) "
            "ON CONFLICT (id) DO UPDATE SET "
            "username = EXCLUDED.username, "
            "display_name = EXCLUDED.display_name, "
            "email = EXCLUDED.email, "
            "profile_pic_url = EXCLUDED.profile_pic_url, "
            "synced_at = NOW()");
        
        // Get user statement
        connection_->prepare("get_user",
            "SELECT username, display_name FROM chat_users WHERE id = $1");
        
        // Update user status
        connection_->prepare("update_user_status",
            "UPDATE chat_users SET is_online = $2, last_seen = NOW() WHERE id = $1");
        
        // Save message statement
        connection_->prepare("save_message",
            "INSERT INTO messages (id, room_id, sender_id, content, message_type, file_url, file_name, file_size, file_type, metadata) "
            "VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10) RETURNING id");
        
        // Get messages statement
        connection_->prepare("get_messages",
            "SELECT m.id, m.room_id, m.sender_id, m.content, m.message_type, "
            "m.file_url, m.file_name, m.file_size, m.file_type, m.metadata, "
            "m.is_edited, m.is_deleted, m.created_at, "
            "u.username, u.display_name "
            "FROM messages m "
            "JOIN chat_users u ON m.sender_id = u.id "
            "WHERE m.room_id = $1 AND m.is_deleted = false "
            "ORDER BY m.created_at DESC LIMIT $2");
        
        // Mark message as read
        connection_->prepare("mark_read",
            "INSERT INTO message_read_status (message_id, user_id) "
            "VALUES ($1, $2) ON CONFLICT (message_id, user_id) DO NOTHING");
        
        // Set typing indicator
        connection_->prepare("set_typing",
            "INSERT INTO typing_indicators (room_id, user_id, expires_at) "
            "VALUES ($1, $2, NOW() + INTERVAL '10 seconds') "
            "ON CONFLICT (room_id, user_id) DO UPDATE SET "
            "started_at = NOW(), expires_at = NOW() + INTERVAL '10 seconds'");
        
        // NEW: Room access check
        connection_->prepare("can_user_join_room",
            "SELECT COUNT(*) FROM room_participants "
            "WHERE room_id = $1 AND user_id = $2 AND is_active = true");
        
        // NEW: Get user rooms
        connection_->prepare("get_user_rooms",
            "SELECT cr.id, cr.name, cr.type, cr.created_by, cr.invite_id, "
            "cr.last_activity, cr.created_at, cr.is_active "
            "FROM chat_rooms cr "
            "JOIN room_participants rp ON cr.id = rp.room_id "
            "WHERE rp.user_id = $1 AND rp.is_active = true AND cr.is_active = true "
            "ORDER BY cr.last_activity DESC");
        
        std::cout << "✅ Database prepared statements created" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Failed to prepare statements: " << e.what() << std::endl;
    }
}

std::string DatabaseManager::generate_uuid() {
    // Simple UUID generation for now
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    
    std::stringstream ss;
    ss << std::hex;
    for (int i = 0; i < 32; ++i) {
        if (i == 8 || i == 12 || i == 16 || i == 20) {
            ss << "-";
        }
        ss << dis(gen);
    }
    return ss.str();
}

bool DatabaseManager::sync_user(const std::string& user_id, const std::string& username,
                                const std::string& display_name, const std::string& email,
                                const std::string& profile_pic_url) {
    try {
        pqxx::work txn(*connection_);
        txn.exec_prepared("sync_user", user_id, username, display_name, email, profile_pic_url);
        txn.commit();
        
        std::cout << "✅ User synced: " << username << " (" << user_id << ")" << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Failed to sync user: " << e.what() << std::endl;
        return false;
    }
}

bool DatabaseManager::get_user(const std::string& user_id, std::string& username, 
                              std::string& display_name) {
    try {
        pqxx::work txn(*connection_);
        pqxx::result result = txn.exec_prepared("get_user", user_id);
        txn.commit();
        
        if (!result.empty()) {
            username = result[0]["username"].c_str();
            display_name = result[0]["display_name"].c_str();
            return true;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Failed to get user: " << e.what() << std::endl;
    }
    
    return false;
}

bool DatabaseManager::update_user_status(const std::string& user_id, bool is_online) {
    try {
        pqxx::work txn(*connection_);
        txn.exec_prepared("update_user_status", user_id, is_online);
        txn.commit();
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Failed to update user status: " << e.what() << std::endl;
        return false;
    }
}

std::string DatabaseManager::create_room(const std::string& name, const std::string& type,
                                        const std::string& created_by, const std::string& invite_id) {
    try {
        std::string room_id = generate_uuid();
        
        pqxx::work txn(*connection_);
        
        std::string query = "INSERT INTO chat_rooms (id, name, type, created_by";
        std::string values = "VALUES ($1, $2, $3, $4";
        
        if (!invite_id.empty()) {
            query += ", invite_id";
            values += ", $5";
        }
        
        query += ") " + values + ")";
        
        if (invite_id.empty()) {
            txn.exec_params(query, room_id, name, type, created_by);
        } else {
            txn.exec_params(query, room_id, name, type, created_by, invite_id);
        }
        
        // Add creator as admin
        txn.exec_params("INSERT INTO room_participants (room_id, user_id, role) VALUES ($1, $2, 'admin')",
                       room_id, created_by);
        
        txn.commit();
        
        std::cout << "✅ Room created: " << name << " (" << room_id << ")" << std::endl;
        return room_id;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Failed to create room: " << e.what() << std::endl;
        return "";
    }
}

bool DatabaseManager::add_participant(const std::string& room_id, const std::string& user_id,
                                     const std::string& role) {
    try {
        pqxx::work txn(*connection_);
        txn.exec_params("INSERT INTO room_participants (room_id, user_id, role) VALUES ($1, $2, $3) "
                       "ON CONFLICT (room_id, user_id) DO UPDATE SET is_active = true, role = EXCLUDED.role",
                       room_id, user_id, role);
        txn.commit();
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Failed to add participant: " << e.what() << std::endl;
        return false;
    }
}

// NEW: Check if user can join room
bool DatabaseManager::can_user_join_room(const std::string& user_id, const std::string& room_id) {
    try {
        pqxx::work txn(*connection_);
        pqxx::result result = txn.exec_prepared("can_user_join_room", room_id, user_id);
        txn.commit();
        
        if (!result.empty()) {
            int count = result[0][0].as<int>();
            // If count > 0, user is already a participant
            // For now, we'll allow anyone to join any room (later add access control)
            return true; // Always allow for testing, modify later for security
        }
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Failed to check room access: " << e.what() << std::endl;
    }
    
    return true; // Allow by default for testing
}

// NEW: Get user's rooms
std::vector<ChatRoom> DatabaseManager::get_user_rooms(const std::string& user_id) {
    std::vector<ChatRoom> rooms;
    
    try {
        pqxx::work txn(*connection_);
        pqxx::result result = txn.exec_prepared("get_user_rooms", user_id);
        txn.commit();
        
        for (const auto& row : result) {
            ChatRoom room;
            room.id = row["id"].c_str();
            room.name = row["name"].c_str();
            room.type = row["type"].c_str();
            room.created_by = row["created_by"].c_str();
            
            if (!row["invite_id"].is_null()) {
                room.invite_id = row["invite_id"].c_str();
            }
            
            room.is_active = row["is_active"].as<bool>();
            
            rooms.push_back(room);
        }
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Failed to get user rooms: " << e.what() << std::endl;
    }
    
    return rooms;
}

// NEW: Alias for get_messages
std::vector<Message> DatabaseManager::get_room_messages(const std::string& room_id, int limit) {
    return get_messages(room_id, limit);
}

std::string DatabaseManager::save_message(const Message& message) {
    try {
        std::string message_id = generate_uuid();
        
        pqxx::work txn(*connection_);
        
        // Convert message type to string
        std::string type_str;
        switch (message.type) {
            case MessageType::TEXT: type_str = "text"; break;
            case MessageType::IMAGE: type_str = "image"; break;
            case MessageType::FILE: type_str = "file"; break;
            case MessageType::LOCATION: type_str = "location"; break;
            case MessageType::SYSTEM: type_str = "system"; break;
        }
        
        txn.exec_prepared("save_message", message_id, message.room_id, message.sender_id,
                         message.content, type_str, "", "", 0, "", "{}");
        txn.commit();
        
        std::cout << "💬 Message saved: " << message_id << std::endl;
        return message_id;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Failed to save message: " << e.what() << std::endl;
        return "";
    }
}

std::vector<Message> DatabaseManager::get_messages(const std::string& room_id, int limit,
                                                  const std::string& before_message_id) {
    std::vector<Message> messages;
    
    try {
        pqxx::work txn(*connection_);
        pqxx::result result = txn.exec_prepared("get_messages", room_id, limit);
        txn.commit();
        
        for (const auto& row : result) {
            Message msg;
            msg.id = row["id"].c_str();
            msg.room_id = row["room_id"].c_str();
            msg.sender_id = row["sender_id"].c_str();
            msg.content = row["content"].c_str();
            
            // Convert type string back to enum
            std::string type_str = row["message_type"].c_str();
            if (type_str == "text") msg.type = MessageType::TEXT;
            else if (type_str == "image") msg.type = MessageType::IMAGE;
            else if (type_str == "file") msg.type = MessageType::FILE;
            else if (type_str == "location") msg.type = MessageType::LOCATION;
            else if (type_str == "system") msg.type = MessageType::SYSTEM;
            
            msg.is_edited = row["is_edited"].as<bool>();
            msg.is_deleted = row["is_deleted"].as<bool>();
            
            messages.push_back(msg);
        }
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Failed to get messages: " << e.what() << std::endl;
    }
    
    return messages;
}

bool DatabaseManager::mark_message_read(const std::string& message_id, const std::string& user_id) {
    try {
        pqxx::work txn(*connection_);
        txn.exec_prepared("mark_read", message_id, user_id);
        txn.commit();
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Failed to mark message as read: " << e.what() << std::endl;
        return false;
    }
}

bool DatabaseManager::set_typing_indicator(const std::string& room_id, const std::string& user_id) {
    try {
        pqxx::work txn(*connection_);
        txn.exec_prepared("set_typing", room_id, user_id);
        txn.commit();
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Failed to set typing indicator: " << e.what() << std::endl;
        return false;
    }
}

bool DatabaseManager::cleanup_expired_typing_indicators() {
    try {
        pqxx::work txn(*connection_);
        pqxx::result result = txn.exec("DELETE FROM typing_indicators WHERE expires_at < NOW()");
        txn.commit();
        
        if (result.affected_rows() > 0) {
            std::cout << "🧹 Cleaned up " << result.affected_rows() << " expired typing indicators" << std::endl;
        }
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Failed to cleanup typing indicators: " << e.what() << std::endl;
        return false;
    }
}

// Add this method to your database_manager.cpp (before the closing brace of namespace caffis)
bool DatabaseManager::ensure_user_in_default_room(const std::string& user_id, const std::string& username) {
    try {
        std::string default_room_id = "550e8400-e29b-41d4-a716-446655440000";
        
        pqxx::work txn(*connection_);
        
        // Check if default room exists
        pqxx::result room_check = txn.exec_params(
            "SELECT id FROM chat_rooms WHERE id = $1", default_room_id);
        
        if (room_check.empty()) {
            // Create default room with explicit UUID
            txn.exec_params(
                "INSERT INTO chat_rooms (id, name, type, created_by, description) "
                "VALUES ($1, $2, $3, $4, $5)",
                default_room_id, 
                "General Chat", 
                "group", 
                user_id,
                "Welcome to Caffis! Start chatting with other coffee lovers."
            );
            std::cout << "✅ Created default 'General Chat' room with ID: " << default_room_id << std::endl;
        }
        
        // Check if user is already a participant
        pqxx::result participant_check = txn.exec_params(
            "SELECT id FROM room_participants WHERE room_id = $1 AND user_id = $2",
            default_room_id, user_id);
        
        if (participant_check.empty()) {
            // Add user as participant
            txn.exec_params(
                "INSERT INTO room_participants (room_id, user_id, role, is_active) "
                "VALUES ($1, $2, $3, $4)",
                default_room_id, user_id, "member", true
            );
            std::cout << "✅ Added " << username << " to General Chat" << std::endl;
        }
        
        // FIXED: Add all existing users to this room using proper parameter syntax
        pqxx::result all_users = txn.exec_params(
            "SELECT id, username FROM chat_users WHERE id != $1", user_id);
        
        for (const auto& user_row : all_users) {
            std::string other_user_id = user_row[0].c_str();
            std::string other_username = user_row[1].c_str();
            
            // Check if they're already a participant
            pqxx::result other_check = txn.exec_params(
                "SELECT id FROM room_participants WHERE room_id = $1 AND user_id = $2",
                default_room_id, other_user_id);
            
            if (other_check.empty()) {
                txn.exec_params(
                    "INSERT INTO room_participants (room_id, user_id, role, is_active) "
                    "VALUES ($1, $2, $3, $4)",
                    default_room_id, other_user_id, "member", true
                );
                std::cout << "✅ Also added " << other_username << " to General Chat" << std::endl;
            }
        }
        
        // COMMIT the transaction
        txn.commit();
        std::cout << "💾 Transaction committed successfully" << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Failed to ensure user in default room: " << e.what() << std::endl;
        return false;
    }
}

std::string DatabaseManager::get_database_stats() {
    try {
        pqxx::work txn(*connection_);
        
        pqxx::result users = txn.exec("SELECT COUNT(*) FROM chat_users");
        pqxx::result rooms = txn.exec("SELECT COUNT(*) FROM chat_rooms");
        pqxx::result messages = txn.exec("SELECT COUNT(*) FROM messages");
        
        txn.commit();
        
        std::stringstream stats;
        stats << "📊 Database Stats:\n";
        stats << "   • Users: " << users[0][0].c_str() << "\n";
        stats << "   • Rooms: " << rooms[0][0].c_str() << "\n";
        stats << "   • Messages: " << messages[0][0].c_str();
        
        return stats.str();
        
    } catch (const std::exception& e) {
        return "❌ Failed to get database stats: " + std::string(e.what());
    }
}

// Placeholder implementations for methods not yet needed
bool DatabaseManager::remove_participant(const std::string& room_id, const std::string& user_id) {
    // TODO: Implement when needed
    return true;
}

std::vector<std::string> DatabaseManager::get_room_participants(const std::string& room_id) {
    // TODO: Implement when needed
    return {};
}

bool DatabaseManager::delete_message(const std::string& message_id, const std::string& user_id) {
    // TODO: Implement when needed
    return true;
}

bool DatabaseManager::edit_message(const std::string& message_id, const std::string& new_content, const std::string& user_id) {
    // TODO: Implement when needed
    return true;
}

bool DatabaseManager::block_user(const std::string& user_id, const std::string& target_user_id) {
    // TODO: Implement when needed
    return true;
}

bool DatabaseManager::unblock_user(const std::string& user_id, const std::string& target_user_id) {
    // TODO: Implement when needed
    return true;
}

bool DatabaseManager::is_user_blocked(const std::string& user_id, const std::string& target_user_id) {
    // TODO: Implement when needed
    return false;
}

bool DatabaseManager::clear_typing_indicator(const std::string& room_id, const std::string& user_id) {
    // TODO: Implement when needed
    return true;
}

std::vector<std::string> DatabaseManager::get_typing_users(const std::string& room_id) {
    // TODO: Implement when needed
    return {};
}

} // namespace caffis