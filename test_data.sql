-- ================================================
-- CAFFIS CHAT SERVICE - TEST DATA
-- ================================================
-- This creates test data that matches the frontend expectations

-- Clear existing test data
DELETE FROM room_participants WHERE room_id IN ('1', '2', '3');
DELETE FROM messages WHERE room_id IN ('1', '2', '3');
DELETE FROM chat_rooms WHERE id IN ('1', '2', '3');
DELETE FROM chat_users WHERE username IN ('testuser1', 'testuser2', 'alice', 'madeup') OR id LIKE '550e8400-e29b-41d4-a716-44665544%';

-- ================================================
-- TEST USERS (matching main app users)
-- ================================================

-- Insert test users with realistic UUIDs (these will be replaced by auto-sync from main app)
-- Note: These are temporary - real users will sync automatically when they first connect
INSERT INTO chat_users (id, username, display_name, email, is_active, is_online) VALUES 
('550e8400-e29b-41d4-a716-446655440001', 'testuser1', 'Test User 1', 'testuser1@caffis.com', true, false),
('550e8400-e29b-41d4-a716-446655440002', 'testuser2', 'Test User 2', 'testuser2@caffis.com', true, false),
('550e8400-e29b-41d4-a716-446655440003', 'alice', 'Alice Johnson', 'alice@caffis.com', true, false),
('550e8400-e29b-41d4-a716-446655440004', 'madeup', 'Made Up User', 'madeup@caffis.com', true, false);

-- ================================================
-- TEST CHAT ROOMS (matching frontend expectations)
-- ================================================

-- Room 1: The main test room that frontend tries to join
INSERT INTO chat_rooms (id, name, description, type, created_by, is_active, last_activity) VALUES 
('1', 'Coffee Meetup - Downtown', 'Main test room for coffee lovers', 'meetup', '550e8400-e29b-41d4-a716-446655440001', true, NOW()),
('2', 'General Chat', 'General discussion room', 'group', '550e8400-e29b-41d4-a716-446655440001', true, NOW()),
('3', 'Private Chat', 'Private conversation', 'private', '550e8400-e29b-41d4-a716-446655440001', true, NOW());

-- ================================================
-- ROOM PARTICIPANTS (all users can access all rooms for testing)
-- ================================================

-- Add all users to Room 1 (the main test room)
INSERT INTO room_participants (room_id, user_id, role, is_active) VALUES 
('1', '550e8400-e29b-41d4-a716-446655440001', 'admin', true),
('1', '550e8400-e29b-41d4-a716-446655440002', 'member', true),
('1', '550e8400-e29b-41d4-a716-446655440003', 'member', true),
('1', '550e8400-e29b-41d4-a716-446655440004', 'member', true);

-- Add users to other rooms
INSERT INTO room_participants (room_id, user_id, role, is_active) VALUES 
('2', '550e8400-e29b-41d4-a716-446655440001', 'admin', true),
('2', '550e8400-e29b-41d4-a716-446655440002', 'member', true),
('2', '550e8400-e29b-41d4-a716-446655440003', 'member', true),

('3', '550e8400-e29b-41d4-a716-446655440001', 'admin', true),
('3', '550e8400-e29b-41d4-a716-446655440004', 'member', true);

-- ================================================
-- SAMPLE MESSAGES FOR TESTING
-- ================================================

-- Add some test messages to room 1
INSERT INTO messages (id, room_id, sender_id, content, message_type, created_at) VALUES 
('msg1', '1', '550e8400-e29b-41d4-a716-446655440001', 'Welcome to the coffee meetup chat!', 'text', NOW() - INTERVAL '2 hours'),
('msg2', '1', '550e8400-e29b-41d4-a716-446655440003', 'Great coffee spot! Thanks for organizing this.', 'text', NOW() - INTERVAL '1 hour'),
('msg3', '1', '550e8400-e29b-41d4-a716-446655440002', 'Looking forward to meeting everyone!', 'text', NOW() - INTERVAL '30 minutes'),
('msg4', '1', '550e8400-e29b-41d4-a716-446655440004', 'What time are we meeting?', 'text', NOW() - INTERVAL '15 minutes');

-- ================================================
-- VERIFICATION QUERIES
-- ================================================

-- Show what we created
SELECT 'USERS' as type, username, display_name, is_online::text as status FROM chat_users
UNION ALL
SELECT 'ROOMS' as type, name, type, is_active::text FROM chat_rooms
ORDER BY type;

-- Show room participant counts
SELECT 
    cr.name as room_name,
    COUNT(rp.user_id) as participant_count
FROM chat_rooms cr 
LEFT JOIN room_participants rp ON cr.id = rp.room_id AND rp.is_active = true
GROUP BY cr.id, cr.name;

COMMIT;
