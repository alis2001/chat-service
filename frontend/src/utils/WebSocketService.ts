import { TokenManager } from './auth';

export interface ChatMessage {
  id: string;
  senderId: string;
  senderName: string;
  content: string;
  timestamp: string;
  roomId: string;
  type: 'text' | 'image' | 'file' | 'system';
}

export interface WebSocketMessage {
  type: string;
  [key: string]: any;
}

export type ConnectionStatus = 'connecting' | 'connected' | 'disconnected' | 'error';

export class WebSocketService {
  private ws: WebSocket | null = null;
  private reconnectAttempts = 0;
  private maxReconnectAttempts = 5;
  private reconnectDelay = 1000;
  private isAuthenticated = false;
  private currentRoom: string | null = null;
  private messageHandlers: ((message: ChatMessage) => void)[] = [];
  private statusHandlers: ((status: ConnectionStatus) => void)[] = [];
  private userJoinHandlers: ((userId: string, username: string) => void)[] = [];

  constructor(private url: string = process.env.REACT_APP_CHAT_SERVER_URL || 'ws://localhost:5004') {}

  // Connection Management
  connect(): Promise<void> {
    return new Promise((resolve, reject) => {
      try {
        console.log('🔌 Connecting to WebSocket server:', this.url);
        this.ws = new WebSocket(this.url);

        this.ws.onopen = () => {
          console.log('✅ WebSocket Connected!');
          this.reconnectAttempts = 0;
          this.notifyStatusHandlers('connected');
          this.authenticateUser().then(resolve).catch(reject);
        };

        this.ws.onmessage = (event) => {
          try {
            const message: WebSocketMessage = JSON.parse(event.data);
            this.handleMessage(message);
          } catch (error) {
            console.error('❌ Failed to parse message:', error);
          }
        };

        this.ws.onclose = (event) => {
          console.log('🔌 WebSocket Disconnected:', event.code, event.reason);
          this.isAuthenticated = false;
          this.currentRoom = null;
          this.notifyStatusHandlers('disconnected');
          
          if (event.code !== 1000) { // Not a normal closure
            this.attemptReconnect();
          }
        };

        this.ws.onerror = (error) => {
          console.error('❌ WebSocket Error:', error);
          this.notifyStatusHandlers('error');
          reject(error);
        };

      } catch (error) {
        console.error('❌ Connection failed:', error);
        reject(error);
      }
    });
  }

  private async authenticateUser(): Promise<void> {
    const token = TokenManager.getToken();
    if (!token) {
      throw new Error('No authentication token found');
    }

    return new Promise((resolve, reject) => {
      const authMessage = {
        type: 'auth',
        token: token
      };

      const timeout = setTimeout(() => {
        reject(new Error('Authentication timeout'));
      }, 10000);

      const originalHandler = this.handleMessage.bind(this);
      this.handleMessage = (message: WebSocketMessage) => {
        if (message.type === 'auth_success') {
          clearTimeout(timeout);
          this.isAuthenticated = true;
          console.log('🔐 Authentication successful:', message.username);
          this.handleMessage = originalHandler;
          resolve();
        } else if (message.type === 'auth_error') {
          clearTimeout(timeout);
          console.error('🔐 Authentication failed:', message.error);
          this.handleMessage = originalHandler;
          reject(new Error(message.error));
        } else {
          originalHandler(message);
        }
      };

      this.send(authMessage);
    });
  }

  // Room Management
  async joinRoom(roomId: string): Promise<void> {
    if (!this.isAuthenticated) {
      throw new Error('Must be authenticated to join room');
    }

    return new Promise((resolve, reject) => {
      const joinMessage = {
        type: 'join_room',
        room_id: roomId
      };

      const timeout = setTimeout(() => {
        reject(new Error('Join room timeout'));
      }, 5000);

      const originalHandler = this.handleMessage.bind(this);
      this.handleMessage = (message: WebSocketMessage) => {
        if (message.type === 'room_joined' && message.room_id === roomId) {
          clearTimeout(timeout);
          this.currentRoom = roomId;
          console.log('🏠 Joined room:', roomId);
          this.handleMessage = originalHandler;
          resolve();
        } else if (message.type === 'error') {
          clearTimeout(timeout);
          this.handleMessage = originalHandler;
          reject(new Error(message.error));
        } else {
          originalHandler(message);
        }
      };

      this.send(joinMessage);
    });
  }

  // Message Sending
  sendMessage(content: string): void {
    if (!this.isAuthenticated || !this.currentRoom) {
      throw new Error('Must be authenticated and in a room to send messages');
    }

    const messageData = {
      type: 'message',
      roomId: this.currentRoom,
      content: content,
      timestamp: new Date().toISOString()
    };

    this.send(messageData);
  }

  // Message Handling
  private handleMessage(message: WebSocketMessage): void {
    console.log('📨 Received:', message.type, message);

    switch (message.type) {
      case 'new_message':
        const chatMessage: ChatMessage = {
          id: message.message_id,
          senderId: message.sender_id,
          senderName: message.sender_name,
          content: message.content,
          timestamp: new Date(parseInt(message.timestamp) * 1000).toLocaleTimeString(),
          roomId: message.room_id,
          type: 'text'
        };
        this.notifyMessageHandlers(chatMessage);
        break;

      case 'rooms_list':
        console.log('📋 Available rooms:', message.rooms);
        // You could emit this to a rooms handler if needed
        // this.notifyRoomsHandlers(message.rooms);
        break;

      case 'user_joined':
        console.log('👋 User joined:', message.username);
        this.notifyUserJoinHandlers(message.user_id, message.username);
        break;

      case 'auth_success':
      case 'auth_error':
      case 'room_joined':
      case 'error':
        // These are handled by specific promise handlers
        break;

      default:
        console.log('❓ Unknown message type:', message.type);
    }
  }

  // Utility Methods
  private send(data: any): void {
    if (this.ws && this.ws.readyState === WebSocket.OPEN) {
      this.ws.send(JSON.stringify(data));
    } else {
      console.error('❌ WebSocket not connected');
    }
  }

  private attemptReconnect(): void {
    if (this.reconnectAttempts < this.maxReconnectAttempts) {
      this.reconnectAttempts++;
      console.log(`🔄 Reconnecting... (${this.reconnectAttempts}/${this.maxReconnectAttempts})`);
      
      setTimeout(() => {
        this.connect().catch(console.error);
      }, this.reconnectDelay * this.reconnectAttempts);
    } else {
      console.error('❌ Max reconnection attempts reached');
    }
  }

  // Event Handlers
  onMessage(handler: (message: ChatMessage) => void): void {
    this.messageHandlers.push(handler);
  }

  onStatusChange(handler: (status: ConnectionStatus) => void): void {
    this.statusHandlers.push(handler);
  }

  onUserJoin(handler: (userId: string, username: string) => void): void {
    this.userJoinHandlers.push(handler);
  }

  private notifyMessageHandlers(message: ChatMessage): void {
    this.messageHandlers.forEach(handler => handler(message));
  }

  private notifyStatusHandlers(status: ConnectionStatus): void {
    this.statusHandlers.forEach(handler => handler(status));
  }

  private notifyUserJoinHandlers(userId: string, username: string): void {
    this.userJoinHandlers.forEach(handler => handler(userId, username));
  }

  // Cleanup
  disconnect(): void {
    if (this.ws) {
      this.ws.close(1000, 'User disconnected');
      this.ws = null;
    }
  }

  // Status
  get isConnected(): boolean {
    return this.ws?.readyState === WebSocket.OPEN && this.isAuthenticated;
  }

  get currentRoomId(): string | null {
    return this.currentRoom;
  }
}