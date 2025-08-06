import React, { useState } from 'react';
import { Send, Paperclip, Smile, MoreVertical, MapPin, Calendar } from 'lucide-react';
import { User } from '../utils/auth';

interface Message {
  id: string;
  senderId: string;
  content: string;
  timestamp: string;
  type: 'text' | 'image' | 'file' | 'system';
  senderName: string;
}

interface ChatAreaProps {
  user: User;
  roomId?: string;
  roomName?: string;
  roomType?: 'private' | 'group' | 'meetup';
  messages: Message[];
  onSendMessage: (content: string) => void;
}

const ChatArea: React.FC<ChatAreaProps> = ({
  user,
  roomId,
  roomName = 'Select a conversation',
  roomType = 'private',
  messages,
  onSendMessage
}) => {
  const [messageInput, setMessageInput] = useState('');

  // Mock messages for demonstration
  const mockMessages: Message[] = [
    {
      id: '1',
      senderId: 'other-user',
      content: 'Hey! Are you still coming to the coffee meetup today?',
      timestamp: '10:30 AM',
      type: 'text',
      senderName: 'Marco Rossi'
    },
    {
      id: '2',
      senderId: user.id,
      content: 'Yes! Looking forward to it. What time are we meeting?',
      timestamp: '10:32 AM',
      type: 'text',
      senderName: `${user.firstName} ${user.lastName}`
    },
    {
      id: '3',
      senderId: 'other-user',
      content: 'Perfect! Let\'s meet at 3 PM at Caffè Centrale. They have amazing espresso!',
      timestamp: '10:35 AM',
      type: 'text',
      senderName: 'Marco Rossi'
    },
    {
      id: '4',
      senderId: 'system',
      content: 'Marco Rossi shared a location: Caffè Centrale',
      timestamp: '10:36 AM',
      type: 'system',
      senderName: 'System'
    }
  ];

  const displayMessages = messages.length > 0 ? messages : mockMessages;

  const handleSendMessage = () => {
    if (messageInput.trim()) {
      onSendMessage(messageInput);
      setMessageInput('');
    }
  };

  const handleKeyPress = (e: React.KeyboardEvent) => {
    if (e.key === 'Enter' && !e.shiftKey) {
      e.preventDefault();
      handleSendMessage();
    }
  };

  if (!roomId) {
    return (
      <div className="chat-area-empty">
        <div className="empty-state">
          <div className="empty-icon">💬</div>
          <h3>Welcome to Caffis Chat</h3>
          <p>Select a conversation to start messaging</p>
          <button className="start-chat-button">
            Start New Conversation
          </button>
        </div>
      </div>
    );
  }

  return (
    <div className="chat-area">
      {/* Chat Header */}
      <div className="chat-area-header">
        <div className="chat-header-info">
          <div className="chat-avatar">
            {roomName.charAt(0).toUpperCase()}
          </div>
          <div className="chat-details">
            <h3 className="chat-name">{roomName}</h3>
            <span className="chat-status">
              {roomType === 'group' ? '5 members' : 'Online'}
            </span>
          </div>
        </div>
        
        <div className="chat-header-actions">
            <button className="header-action-button" title="Share Location">
                <MapPin size={20} />
            </button>
            <button className="header-action-button" title="Event Details">
                <Calendar size={20} />
            </button>
            <button className="header-action-button" title="More Options">
                <MoreVertical size={20} />
            </button>
        </div>
      </div>

      {/* Messages Container */}
      <div className="messages-container">
        {displayMessages.map((message) => (
          <div
            key={message.id}
            className={`message ${message.senderId === user.id ? 'sent' : 'received'} ${message.type}`}
          >
            {message.senderId !== user.id && message.type !== 'system' && (
              <div className="message-avatar">
                {message.senderName.charAt(0).toUpperCase()}
              </div>
            )}
            
            <div className="message-content">
              {message.senderId !== user.id && message.type !== 'system' && (
                <div className="message-sender">{message.senderName}</div>
              )}
              <div className="message-text">{message.content}</div>
              <div className="message-time">{message.timestamp}</div>
            </div>
          </div>
        ))}
      </div>

      {/* Message Input */}
      <div className="message-input-container">
        <div className="message-input-wrapper">
          <button className="input-action-button" title="Attach File">
            <Paperclip size={20} />
          </button>
          
          <textarea
            value={messageInput}
            onChange={(e) => setMessageInput(e.target.value)}
            onKeyPress={handleKeyPress}
            placeholder="Type a message..."
            className="message-input"
            rows={1}
          />
          
          <button className="input-action-button" title="Emoji">
            <Smile size={20} />
          </button>
          
          <button 
            className="send-button"
            onClick={handleSendMessage}
            disabled={!messageInput.trim()}
            title="Send Message"
          >
            <Send size={20} />
          </button>
        </div>
      </div>
    </div>
  );
};

export default ChatArea;