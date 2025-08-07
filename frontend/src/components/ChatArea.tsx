import React, { useState, useEffect, useRef } from 'react';
import { Send, Paperclip, Smile, MoreVertical, MapPin, Calendar } from 'lucide-react';
import { User } from '../utils/auth';
import { ChatMessage } from '../utils/WebSocketService';

interface ChatAreaProps {
  user: User;
  roomId?: string;
  roomName?: string;
  roomType?: 'private' | 'group' | 'meetup';
  messages: ChatMessage[];
  onSendMessage: (content: string) => void;
  isConnected?: boolean;
  isJoiningRoom?: boolean;
}

const ChatArea: React.FC<ChatAreaProps> = ({
  user,
  roomId,
  roomName = 'Select a conversation',
  roomType = 'private',
  messages,
  onSendMessage,
  isConnected = false,
  isJoiningRoom = false
}) => {
  const [messageInput, setMessageInput] = useState('');
  const messagesEndRef = useRef<HTMLDivElement>(null);

  // Auto-scroll to bottom when new messages arrive
  useEffect(() => {
    messagesEndRef.current?.scrollIntoView({ behavior: 'smooth' });
  }, [messages]);

  // Handle sending messages
  const handleSendMessage = () => {
    if (!messageInput.trim() || !isConnected) {
      return;
    }

    onSendMessage(messageInput);
    setMessageInput('');
  };

  // Handle Enter key press
  const handleKeyPress = (e: React.KeyboardEvent) => {
    if (e.key === 'Enter' && !e.shiftKey) {
      e.preventDefault();
      handleSendMessage();
    }
  };

  // Show empty state if no room selected
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
              {isJoiningRoom ? (
                <span className="joining-status">Joining room...</span>
              ) : roomType === 'group' ? (
                '5 members'
              ) : (
                isConnected ? 'Online' : 'Offline'
              )}
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
        {isJoiningRoom && (
          <div className="joining-room-indicator">
            <div className="joining-spinner"></div>
            <span>Joining room...</span>
          </div>
        )}

        {messages.length === 0 && !isJoiningRoom && (
          <div className="no-messages-state">
            <div className="no-messages-icon">☕</div>
            <p>Start the conversation! Say hello to your coffee buddies.</p>
          </div>
        )}

        {messages.map((message) => (
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

        {/* Auto-scroll target */}
        <div ref={messagesEndRef} />
      </div>

      {/* Message Input */}
      <div className="message-input-container">
        <div className="message-input-wrapper">
          <button 
            className="input-action-button" 
            title="Attach File"
            disabled={!isConnected}
          >
            <Paperclip size={20} />
          </button>
          
          <textarea
            value={messageInput}
            onChange={(e) => setMessageInput(e.target.value)}
            onKeyPress={handleKeyPress}
            placeholder={
              !isConnected 
                ? "Connecting..." 
                : isJoiningRoom 
                ? "Joining room..."
                : "Type a message..."
            }
            className="message-input"
            rows={1}
            disabled={!isConnected || isJoiningRoom}
          />
          
          <button 
            className="input-action-button" 
            title="Emoji"
            disabled={!isConnected}
          >
            <Smile size={20} />
          </button>
          
          <button 
            className="send-button"
            onClick={handleSendMessage}
            disabled={!messageInput.trim() || !isConnected || isJoiningRoom}
            title={
              !isConnected 
                ? "Not connected" 
                : !messageInput.trim() 
                ? "Enter a message"
                : "Send Message"
            }
          >
            <Send size={20} />
          </button>
        </div>

        {!isConnected && (
          <div className="connection-warning">
            <span>⚠️ Not connected to chat server</span>
          </div>
        )}
      </div>
    </div>
  );
};

export default ChatArea;