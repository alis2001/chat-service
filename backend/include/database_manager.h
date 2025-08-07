#pragma once

#include <pqxx/pqxx>
#include <memory>
#include <string>
#include <vector>
#include "message_types.h"

namespace caffis {

class DatabaseManager {
private:
    std::unique_ptr<pqxx::connection> connection_;
    std::string connection_string_;
    bool is_connected_;

public:
    explicit DatabaseManager(const std::string& connection_string);
    ~DatabaseManager();
    
    // Connection management
    bool connect();
    void disconnect();
    bool is_connected() const { return is_connected_; }
    bool test_connection();
    
    // User operations
    bool sync_user(const std::string& user_id, const std::string& username, 
                   const std::string& display_name, const std::string& email,
                   const std::string& profile_pic_url = "");
    bool get_user(const std::string& user_id, std::string& username, 
                  std::string& display_name);
    bool update_user_status(const std::string& user_id, bool is_online);
    
    // Room operations
    std::string create_room(const std::string& name, const std::string& type, 
                           const std::string& created_by, const std::string& invite_id = "");
    bool add_participant(const std::string& room_id, const std::string& user_id, 
                        const std::string& role = "member");
    bool remove_participant(const std::string& room_id, const std::string& user_id);
    std::vector<std::string> get_room_participants(const std::string& room_id);
    
    // NEW: Room access and user rooms
    bool can_user_join_room(const std::string& user_id, const std::string& room_id);
    std::vector<ChatRoom> get_user_rooms(const std::string& user_id);
    std::vector<Message> get_room_messages(const std::string& room_id, int limit = 50);
    
    // Message operations
    std::string save_message(const Message& message);
    std::vector<Message> get_messages(const std::string& room_id, int limit = 50, 
                                     const std::string& before_message_id = "");
    bool mark_message_read(const std::string& message_id, const std::string& user_id);
    bool delete_message(const std::string& message_id, const std::string& user_id);
    bool edit_message(const std::string& message_id, const std::string& new_content, 
                     const std::string& user_id);
    
    // User relationships
    bool block_user(const std::string& user_id, const std::string& target_user_id);
    bool unblock_user(const std::string& user_id, const std::string& target_user_id);
    bool is_user_blocked(const std::string& user_id, const std::string& target_user_id);
    
    // Typing indicators
    bool set_typing_indicator(const std::string& room_id, const std::string& user_id);
    bool clear_typing_indicator(const std::string& room_id, const std::string& user_id);
    std::vector<std::string> get_typing_users(const std::string& room_id);
    
    // Health and maintenance
    bool cleanup_expired_typing_indicators();
    std::string get_database_stats();
    

    bool ensure_user_in_default_room(const std::string& user_id, const std::string& username);

private:
    std::string generate_uuid();
    pqxx::result execute_query(const std::string& query);
    pqxx::result execute_prepared(const std::string& name, 
                                  const std::vector<std::string>& params);
    void prepare_statements();
};

} // namespace caffis