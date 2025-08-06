// ================================================
// CAFFIS CHAT SERVICE - AUTHENTICATION UTILITIES
// ================================================

export interface User {
  id: string;
  firstName: string;
  lastName: string;
  username: string;
  email?: string;
  phoneNumber?: string;
  profilePic?: string;
}

export interface AuthResult {
  success: boolean;
  user?: User;
  token?: string;
  error?: string;
}

export class TokenManager {
  private static readonly TOKEN_KEY = 'caffis_chat_token';
  
  static getTokenFromURL(): string | null {
    const urlParams = new URLSearchParams(window.location.search);
    return urlParams.get('token');
  }
  
  static saveToken(token: string): boolean {
    if (token && token !== 'null') {
      localStorage.setItem(this.TOKEN_KEY, token);
      localStorage.setItem('caffis_auth_token', token);
      return true;
    }
    return false;
  }
  
  static getToken(): string | null {
    return localStorage.getItem(this.TOKEN_KEY) || 
           localStorage.getItem('caffis_auth_token');
  }
  
  static clearToken(): void {
    localStorage.removeItem(this.TOKEN_KEY);
    localStorage.removeItem('caffis_auth_token');
  }
  
  static isValidToken(token: string | null): boolean {
    if (!token || token === 'null') return false;
    
    try {
      const parts = token.split('.');
      return parts.length === 3;
    } catch (error) {
      return false;
    }
  }
}

export class ChatAuthAPI {
  private static readonly BASE_URL = process.env.REACT_APP_API_URL || 'http://localhost:5000';
  
  static async fetchUserData(token: string): Promise<User> {
    const response = await fetch(`${this.BASE_URL}/api/user/profile`, {
      headers: {
        'Authorization': `Bearer ${token}`,
        'Content-Type': 'application/json',
      },
    });

    if (!response.ok) {
      throw new Error('Failed to fetch user data from main app');
    }

    const data = await response.json();
    return data.user;
  }
}

export class ChatAuth {
  static async initialize(): Promise<AuthResult> {
    try {
      console.log('🔐 Initializing chat authentication...');
      
      let token = TokenManager.getTokenFromURL();
      
      if (token && TokenManager.isValidToken(token)) {
        console.log('✅ Token found in URL, saving...');
        TokenManager.saveToken(token);
        
        const url = new URL(window.location.href);
        url.searchParams.delete('token');
        window.history.replaceState({}, document.title, url.pathname);
      } else {
        token = TokenManager.getToken();
        console.log('🔍 Checking stored token...');
      }
      
      if (!TokenManager.isValidToken(token)) {
        return {
          success: false,
          error: 'No valid authentication token found'
        };
      }
      
      console.log('👤 Fetching user data from main app...');
      const userData = await ChatAuthAPI.fetchUserData(token!);
      
      console.log('🎉 Chat authentication complete!');
      return {
        success: true,
        user: userData,
        token: token!
      };
      
    } catch (error) {
      console.error('❌ Chat authentication failed:', error);
      TokenManager.clearToken();
      
      return {
        success: false,
        error: error instanceof Error ? error.message : 'Authentication failed'
      };
    }
  }
}