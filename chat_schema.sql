-- ================================================
-- CAFFIS CHAT SERVICE DATABASE SCHEMA
-- ================================================
-- Database: chat_service
-- User: chat_user
-- Port: 5434

-- Enable UUID extension
CREATE EXTENSION IF NOT EXISTS "uuid-ossp";

-- ================================================
-- USER SYNC TABLE (Cache from main app)
-- ================================================
CREATE TABLE chat_users (
    id UUID PRIMARY KEY,                    -- Same ID as main app
    username VARCHAR(50) NOT NULL,
    display_name VARCHAR(100) NOT NULL,
    email VARCHAR(255) NOT NULL,
    profile_pic_url TEXT,
    is_active BOOLEAN DEFAULT true,
    is_online BOOLEAN DEFAULT false,
    last_seen TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    synced_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT NOW()
);

-- ================================================
-- CHAT ROOMS
-- ================================================
CREATE TABLE chat_rooms (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    name VARCHAR(255),
    description TEXT,
    type VARCHAR(20) NOT NULL DEFAULT 'private',  -- 'private', 'group', 'meetup'
    invite_id UUID,                               -- Link to meetup invite
    created_by UUID NOT NULL,
    is_active BOOLEAN DEFAULT true,
    last_activity TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    
    FOREIGN KEY (created_by) REFERENCES chat_users(id),
    CONSTRAINT valid_room_type CHECK (type IN ('private', 'group', 'meetup'))
);

-- ================================================
-- ROOM PARTICIPANTS
-- ================================================
CREATE TABLE room_participants (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    room_id UUID NOT NULL,
    user_id UUID NOT NULL,
    role VARCHAR(20) DEFAULT 'member',            -- 'admin', 'moderator', 'member'
    joined_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    left_at TIMESTAMP WITH TIME ZONE,
    is_muted BOOLEAN DEFAULT false,
    is_active BOOLEAN DEFAULT true,
    
    FOREIGN KEY (room_id) REFERENCES chat_rooms(id) ON DELETE CASCADE,
    FOREIGN KEY (user_id) REFERENCES chat_users(id),
    UNIQUE(room_id, user_id),
    CONSTRAINT valid_participant_role CHECK (role IN ('admin', 'moderator', 'member'))
);

-- ================================================
-- MESSAGES (Core chat functionality)
-- ================================================
CREATE TABLE messages (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    room_id UUID NOT NULL,
    sender_id UUID NOT NULL,
    content TEXT,
    message_type VARCHAR(20) DEFAULT 'text',      -- 'text', 'image', 'file', 'location', 'system'
    file_url TEXT,                               -- MinIO file URL
    file_name VARCHAR(255),
    file_size BIGINT,
    file_type VARCHAR(50),
    metadata JSONB,                              -- Additional data (location coords, etc.)
    is_edited BOOLEAN DEFAULT false,
    is_deleted BOOLEAN DEFAULT false,
    edited_at TIMESTAMP WITH TIME ZONE,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    
    FOREIGN KEY (room_id) REFERENCES chat_rooms(id) ON DELETE CASCADE,
    FOREIGN KEY (sender_id) REFERENCES chat_users(id),
    CONSTRAINT valid_message_type CHECK (message_type IN ('text', 'image', 'file', 'location', 'system'))
);

-- ================================================
-- MESSAGE READ STATUS
-- ================================================
CREATE TABLE message_read_status (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    message_id UUID NOT NULL,
    user_id UUID NOT NULL,
    read_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    
    FOREIGN KEY (message_id) REFERENCES messages(id) ON DELETE CASCADE,
    FOREIGN KEY (user_id) REFERENCES chat_users(id),
    UNIQUE(message_id, user_id)
);

-- ================================================
-- USER RELATIONSHIPS (Block/Unblock)
-- ================================================
CREATE TABLE user_relationships (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    user_id UUID NOT NULL,
    target_user_id UUID NOT NULL,
    relationship_type VARCHAR(20) NOT NULL,       -- 'blocked', 'favorite', 'muted'
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    
    FOREIGN KEY (user_id) REFERENCES chat_users(id),
    FOREIGN KEY (target_user_id) REFERENCES chat_users(id),
    UNIQUE(user_id, target_user_id, relationship_type),
    CONSTRAINT valid_relationship_type CHECK (relationship_type IN ('blocked', 'favorite', 'muted')),
    CONSTRAINT no_self_relationship CHECK (user_id != target_user_id)
);

-- ================================================
-- TYPING INDICATORS (Real-time status)
-- ================================================
CREATE TABLE typing_indicators (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    room_id UUID NOT NULL,
    user_id UUID NOT NULL,
    started_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    expires_at TIMESTAMP WITH TIME ZONE DEFAULT NOW() + INTERVAL '10 seconds',
    
    FOREIGN KEY (room_id) REFERENCES chat_rooms(id) ON DELETE CASCADE,
    FOREIGN KEY (user_id) REFERENCES chat_users(id),
    UNIQUE(room_id, user_id)
);

-- ================================================
-- INDEXES FOR PERFORMANCE
-- ================================================

-- Messages indexes (time-series optimization)
CREATE INDEX idx_messages_room_created ON messages(room_id, created_at DESC);
CREATE INDEX idx_messages_sender ON messages(sender_id);
CREATE INDEX idx_messages_type ON messages(message_type);
CREATE INDEX idx_messages_created ON messages(created_at DESC);

-- Room participants indexes
CREATE INDEX idx_room_participants_room ON room_participants(room_id);
CREATE INDEX idx_room_participants_user ON room_participants(user_id);
CREATE INDEX idx_room_participants_active ON room_participants(room_id, is_active);

-- Chat rooms indexes
CREATE INDEX idx_chat_rooms_created_by ON chat_rooms(created_by);
CREATE INDEX idx_chat_rooms_type ON chat_rooms(type);
CREATE INDEX idx_chat_rooms_active ON chat_rooms(is_active);
CREATE INDEX idx_chat_rooms_invite ON chat_rooms(invite_id);
CREATE INDEX idx_chat_rooms_activity ON chat_rooms(last_activity DESC);

-- Read status indexes
CREATE INDEX idx_message_read_status_message ON message_read_status(message_id);
CREATE INDEX idx_message_read_status_user ON message_read_status(user_id);

-- User indexes
CREATE INDEX idx_chat_users_username ON chat_users(username);
CREATE INDEX idx_chat_users_online ON chat_users(is_online);
CREATE INDEX idx_chat_users_active ON chat_users(is_active);

-- Relationships indexes
CREATE INDEX idx_user_relationships_user ON user_relationships(user_id);
CREATE INDEX idx_user_relationships_target ON user_relationships(target_user_id);
CREATE INDEX idx_user_relationships_type ON user_relationships(relationship_type);

-- Typing indicators indexes (for cleanup)
CREATE INDEX idx_typing_indicators_expires ON typing_indicators(expires_at);
CREATE INDEX idx_typing_indicators_room ON typing_indicators(room_id);

-- ================================================
-- FUNCTIONS AND TRIGGERS
-- ================================================

-- Update updated_at timestamp function
CREATE OR REPLACE FUNCTION update_updated_at_column()
RETURNS TRIGGER AS $$
BEGIN
    NEW.updated_at = NOW();
    RETURN NEW;
END;
$$ language 'plpgsql';

-- Apply updated_at trigger to relevant tables
CREATE TRIGGER update_chat_users_updated_at 
    BEFORE UPDATE ON chat_users 
    FOR EACH ROW EXECUTE FUNCTION update_updated_at_column();

CREATE TRIGGER update_chat_rooms_updated_at 
    BEFORE UPDATE ON chat_rooms 
    FOR EACH ROW EXECUTE FUNCTION update_updated_at_column();

-- Clean up expired typing indicators function
CREATE OR REPLACE FUNCTION cleanup_expired_typing_indicators()
RETURNS void AS $$
BEGIN
    DELETE FROM typing_indicators WHERE expires_at < NOW();
END;
$$ LANGUAGE plpgsql;

-- ================================================
-- PARTITIONING FOR SCALABILITY (Optional)
-- ================================================

-- Partition messages table by month for better performance with large datasets
-- (Uncomment when you have millions of messages)

/*
-- Convert messages table to partitioned table
CREATE TABLE messages_partitioned (
    LIKE messages INCLUDING ALL
) PARTITION BY RANGE (created_at);

-- Create monthly partitions (example)
CREATE TABLE messages_2025_01 PARTITION OF messages_partitioned
    FOR VALUES FROM ('2025-01-01') TO ('2025-02-01');

CREATE TABLE messages_2025_02 PARTITION OF messages_partitioned
    FOR VALUES FROM ('2025-02-01') TO ('2025-03-01');
*/

-- ================================================
-- SAMPLE DATA FOR TESTING
-- ================================================

-- Insert a test user (will be replaced by sync from main app)
INSERT INTO chat_users (id, username, display_name, email) VALUES 
(uuid_generate_v4(), 'testuser1', 'Test User 1', 'test1@caffis.com'),
(uuid_generate_v4(), 'testuser2', 'Test User 2', 'test2@caffis.com');

-- Insert a test chat room
INSERT INTO chat_rooms (id, name, type, created_by) VALUES 
(uuid_generate_v4(), 'General Chat', 'group', (SELECT id FROM chat_users LIMIT 1));

COMMIT;