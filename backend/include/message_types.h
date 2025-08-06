#pragma once

#include <string>
#include <chrono>

namespace caffis {

enum class MessageType {
    TEXT,
    IMAGE,
    FILE,
    LOCATION,
    SYSTEM
};

struct Message {
    std::string id;
    std::string room_id;
    std::string sender_id;
    std::string content;
    MessageType type;
    std::chrono::system_clock::time_point timestamp;
    bool is_edited = false;
    bool is_deleted = false;
    
    // Optional file-related fields
    std::string file_url;
    std::string file_name;
    size_t file_size = 0;
    std::string file_type;
    std::string metadata; // JSON string
    
    // Helper method to convert to JSON-like string
    std::string to_string() const {
        return "Message{id:" + id + ", content:" + content + "}";
    }
};

struct ChatRoom {
    std::string id;
    std::string name;
    std::string type;
    std::string created_by;
    std::string invite_id;
    std::chrono::system_clock::time_point last_activity;
    std::chrono::system_clock::time_point created_at;
    bool is_active = true;
    
    std::string to_string() const {
        return "ChatRoom{id:" + id + ", name:" + name + ", type:" + type + "}";
    }
};

struct ChatUser {
    std::string id;
    std::string username;
    std::string display_name;
    std::string email;
    std::string profile_pic_url;
    bool is_active = true;
    bool is_online = false;
    std::chrono::system_clock::time_point last_seen;
    std::chrono::system_clock::time_point synced_at;
};

} // namespace caffis