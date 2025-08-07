import React, { useState, useEffect, useRef } from 'react';
import { User } from '../utils/auth';
import { WebSocketService, ChatMessage, ConnectionStatus } from '../utils/WebSocketService';
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

interface ChatInterfaceProps {
  user: User;
  onLogout: () => void;
}

const ChatInterface: React.FC<ChatInterfaceProps> = ({ user, onLogout }) => {
  const [selectedRoomId, setSelectedRoomId] = useState<string>('1');
  const [chatRooms] = useState<ChatRoom[]>([
    {
      id: '1',
      name: 'Coffee Meetup - Downtown',
      type: 'meetup',
      lastMessage: 'Great coffee spot!',
      lastMessageTime: '2 min ago',
      unreadCount: 0,
      isOnline: true
    }
  ]);
  
  const [messages, setMessages] = useState<ChatMessage[]>([]);
  const [connectionStatus, setConnectionStatus] = useState<ConnectionStatus>('disconnected');
  const [isJoiningRoom, setIsJoiningRoom] = useState(false);
  
  const webSocketService = useRef<WebSocketService | null>(null);

  // Initialize WebSocket connection
  useEffect(() => {
    console.log('🚀 Initializing WebSocket service...');
    webSocketService.current = new WebSocketService();

    // Set up event handlers
    webSocketService.current.onMessage((message: ChatMessage) => {
      console.log('💬 New message received:', message);
      setMessages(prev => [...prev, message]);
    });

    webSocketService.current.onStatusChange((status: ConnectionStatus) => {
      console.log('🔌 Connection status changed:', status);
      setConnectionStatus(status);
    });

    webSocketService.current.onUserJoin((userId: string, username: string) => {
      console.log('👋 User joined:', username);
      // You could show a system message here
    });

    // Connect to WebSocket
    webSocketService.current.connect()
      .then(() => {
        console.log('✅ WebSocket service initialized successfully');
        // Auto-join the selected room
        if (selectedRoomId) {
          joinRoom(selectedRoomId);
        }
      })
      .catch((error) => {
        console.error('❌ Failed to initialize WebSocket service:', error);
      });

    // Cleanup on unmount
    return () => {
      if (webSocketService.current) {
        webSocketService.current.disconnect();
      }
    };
  }, []);

  // Join room function
  const joinRoom = async (roomId: string) => {
    if (!webSocketService.current?.isConnected) {
      console.log('⏳ WebSocket not connected, waiting...');
      return;
    }

    setIsJoiningRoom(true);
    try {
      await webSocketService.current.joinRoom(roomId);
      console.log(`✅ Successfully joined room: ${roomId}`);
      
      // Clear messages when switching rooms
      setMessages([]);
      
      // TODO: Load message history from backend API
      // const messageHistory = await fetchMessageHistory(roomId);
      // setMessages(messageHistory);
      
    } catch (error) {
      console.error(`❌ Failed to join room ${roomId}:`, error);
    } finally {
      setIsJoiningRoom(false);
    }
  };

  // Handle room selection
  const handleRoomSelect = (roomId: string) => {
    console.log('🏠 Selecting room:', roomId);
    setSelectedRoomId(roomId);
    joinRoom(roomId);
  };

  // Handle new chat
  const handleNewChat = () => {
    console.log('💬 Starting new chat...');
    // TODO: Implement new chat functionality
    // This could open a modal to select users or create a new room
  };

  // Handle sending messages
  const handleSendMessage = (content: string) => {
    if (!webSocketService.current?.isConnected) {
      console.error('❌ Cannot send message: WebSocket not connected');
      return;
    }

    try {
      webSocketService.current.sendMessage(content);
      console.log('✅ Message sent:', content);
    } catch (error) {
      console.error('❌ Failed to send message:', error);
    }
  };

  // Get connection status display
  const getConnectionStatusText = (): string => {
    switch (connectionStatus) {
      case 'connecting':
        return 'Connecting...';
      case 'connected':
        return 'Connected';
      case 'disconnected':
        return 'Disconnected';
      case 'error':
        return 'Connection Error';
      default:
        return 'Unknown';
    }
  };

  const selectedRoom = chatRooms.find(room => room.id === selectedRoomId);

  return (
    <div className="professional-chat-layout">
      {/* Connection Status Indicator */}
      {connectionStatus !== 'connected' && (
        <div className={`connection-status-bar ${connectionStatus}`}>
          <span className="status-indicator"></span>
          {getConnectionStatusText()}
          {isJoiningRoom && ' • Joining room...'}
        </div>
      )}

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
        isConnected={connectionStatus === 'connected'}
        isJoiningRoom={isJoiningRoom}
      />
    </div>
  );
};

export default ChatInterface;