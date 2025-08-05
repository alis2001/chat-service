#pragma once

#include <string>
#include <chrono>
#include <nlohmann/json.hpp>

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
    
    nlohmann::json to_json() const;
    static Message from_json(const nlohmann::json& j);
};

struct ChatRoom {
    std::string id;
    std::string name;
    std::string type;
    std::chrono::system_clock::time_point last_activity;
    bool is_active = true;
    
    nlohmann::json to_json() const;
};

} // namespace caffis
