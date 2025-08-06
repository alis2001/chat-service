import React from 'react';
import { Plus, Search, Coffee, Users, MessageCircle } from 'lucide-react';
import { User } from '../utils/auth';

interface ChatRoom {
  id: string;
  name: string;
  type: 'private' | 'group' | 'meetup';
  lastMessage?: string;
  lastMessageTime?: string;
  unreadCount?: number;
  isOnline?: boolean;
  avatar?: string;
}

interface ChatSidebarProps {
  user: User;
  chatRooms: ChatRoom[];
  selectedRoomId?: string;
  onRoomSelect: (roomId: string) => void;
  onNewChat: () => void;
}

const ChatSidebar: React.FC<ChatSidebarProps> = ({
  user,
  chatRooms,
  selectedRoomId,
  onRoomSelect,
  onNewChat
}) => {
  // Mock data for demonstration
  const mockRooms: ChatRoom[] = [
    {
      id: '1',
      name: 'Coffee Meetup - Downtown',
      type: 'meetup',
      lastMessage: 'See you at 3pm!',
      lastMessageTime: '2m ago',
      unreadCount: 2,
      isOnline: true
    },
    {
      id: '2', 
      name: 'Marco Rossi',
      type: 'private',
      lastMessage: 'Great coffee recommendation!',
      lastMessageTime: '15m ago',
      unreadCount: 0,
      isOnline: true
    },
    {
      id: '3',
      name: 'Coffee Lovers Group',
      type: 'group',
      lastMessage: 'Anna: Anyone tried the new place?',
      lastMessageTime: '1h ago',
      unreadCount: 5,
      isOnline: false
    }
  ];

  const rooms = chatRooms.length > 0 ? chatRooms : mockRooms;

  const getRoomIcon = (type: string) => {
    switch (type) {
      case 'meetup': return <Coffee size={20} />;
      case 'group': return <Users size={20} />;
      case 'private': return <MessageCircle size={20} />;
      default: return <MessageCircle size={20} />;
    }
  };

  const getRoomTypeColor = (type: string) => {
    switch (type) {
      case 'meetup': return 'var(--gradient-accent)';
      case 'group': return 'var(--gradient-secondary)';  
      case 'private': return 'var(--gradient-primary)';
      default: return 'var(--gradient-primary)';
    }
  };

  return (
    <div className="chat-sidebar">
      {/* Sidebar Header */}
      <div className="sidebar-header">
        <div className="sidebar-user">
          <div className="user-avatar">
            {user.firstName?.charAt(0).toUpperCase()}
          </div>
          <div className="user-info">
            <span className="user-name">{user.firstName} {user.lastName}</span>
            <span className="user-status">Online</span>
          </div>
        </div>
        
        <button 
          className="new-chat-button"
          onClick={onNewChat}
          title="New Chat"
        >
          <Plus size={20} />
        </button>
      </div>

      {/* Search Bar */}
      <div className="search-container">
        <div className="search-input-wrapper">
          <Search size={16} className="search-icon" />
          <input 
            type="text" 
            placeholder="Search conversations..."
            className="search-input"
          />
        </div>
      </div>

      {/* Chat Rooms List */}
      <div className="chat-rooms-list">
        {rooms.map((room) => (
          <div
            key={room.id}
            className={`chat-room-item ${selectedRoomId === room.id ? 'selected' : ''}`}
            onClick={() => onRoomSelect(room.id)}
          >
            <div className="room-avatar">
              <div 
                className="room-icon"
                style={{ background: getRoomTypeColor(room.type) }}
              >
                {getRoomIcon(room.type)}
              </div>
              {room.isOnline && (
                <div className="online-indicator" />
              )}
            </div>
            
            <div className="room-content">
              <div className="room-header">
                <span className="room-name">{room.name}</span>
                <span className="room-time">{room.lastMessageTime}</span>
              </div>
              
              <div className="room-message">
                <span className="last-message">{room.lastMessage}</span>
                {room.unreadCount && room.unreadCount > 0 && (
                  <span className="unread-count">{room.unreadCount}</span>
                )}
              </div>
            </div>
          </div>
        ))}
      </div>

      {/* Sidebar Footer */}
      <div className="sidebar-footer">
        <div className="footer-stats">
          <span className="stat-item">
            <Coffee size={16} />
            {rooms.filter(r => r.type === 'meetup').length} Meetups
          </span>
          <span className="stat-item">
            <MessageCircle size={16} />
            {rooms.filter(r => r.unreadCount && r.unreadCount > 0).length} Unread
          </span>
        </div>
      </div>
    </div>
  );
};

export default ChatSidebar;