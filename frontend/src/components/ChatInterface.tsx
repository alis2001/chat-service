import React, { useState } from 'react';
import { User } from '../utils/auth';
import ChatSidebar from './ChatSidebar';
import ChatArea from './ChatArea';

interface ChatRoom {
  id: string;
  name: string;
  type: 'private' | 'group' | 'meetup';
  lastMessage?: string;
  lastMessageTime?: string;
  unreadCount?: number;
  isOnline?: boolean;
}

interface Message {
  id: string;
  senderId: string;
  content: string;
  timestamp: string;
  type: 'text' | 'image' | 'file' | 'system';
  senderName: string;
}

interface ChatInterfaceProps {
  user: User;
  onLogout: () => void;
}

const ChatInterface: React.FC<ChatInterfaceProps> = ({ user, onLogout }) => {
  const [selectedRoomId, setSelectedRoomId] = useState<string>('1');
  const [chatRooms] = useState<ChatRoom[]>([]);
  const [messages] = useState<Message[]>([]);

  const handleRoomSelect = (roomId: string) => {
    setSelectedRoomId(roomId);
    console.log('Selected room:', roomId);
  };

  const handleNewChat = () => {
    console.log('Starting new chat...');
    // TODO: Implement new chat functionality
  };

  const handleSendMessage = (content: string) => {
    console.log('Sending message:', content);
    
    // Test WebSocket connection
    try {
      const ws = new WebSocket(process.env.REACT_APP_CHAT_SERVER_URL || 'ws://localhost:5004');
      
      ws.onopen = () => {
        console.log('✅ WebSocket Connected!');
        ws.send(JSON.stringify({
          type: 'message',
          roomId: selectedRoomId,
          content: content,
          user: user.firstName,
          timestamp: new Date().toISOString()
        }));
      };
      
      ws.onmessage = (msg) => {
        console.log('📨 Received:', msg.data);
      };
      
      ws.onerror = (error) => {
        console.error('❌ WebSocket Error:', error);
      };
    } catch (error) {
      console.error('❌ Connection error:', error);
    }
  };

  const selectedRoom = chatRooms.find(room => room.id === selectedRoomId);

  return (
    <div className="professional-chat-layout">
      <ChatSidebar
        user={user}
        chatRooms={chatRooms}
        selectedRoomId={selectedRoomId}
        onRoomSelect={handleRoomSelect}
        onNewChat={handleNewChat}
      />
      
      <ChatArea
        user={user}
        roomId={selectedRoomId}
        roomName={selectedRoom?.name || 'Coffee Meetup - Downtown'}
        roomType={selectedRoom?.type || 'meetup'}
        messages={messages}
        onSendMessage={handleSendMessage}
      />
    </div>
  );
};

export default ChatInterface;