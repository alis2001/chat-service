import React, { useState, useEffect } from 'react';
import { ChatAuth, TokenManager, User } from './utils/auth';
import LoadingScreen from './components/LoadingScreen';
import ErrorScreen from './components/ErrorScreen';
import ChatInterface from './components/ChatInterface';
import './styles/App.css';

interface AuthState {
  loading: boolean;
  authenticated: boolean;
  user: User | null;
  error: string | null;
}

const App: React.FC = () => {
  const [authState, setAuthState] = useState<AuthState>({
    loading: true,
    authenticated: false,
    user: null,
    error: null
  });

  const initializeAuth = async (): Promise<void> => {
    setAuthState(prev => ({ 
      ...prev, 
      loading: true, 
      error: null 
    }));
    
    try {
      const result = await ChatAuth.initialize();
      
      if (result.success && result.user) {
        setAuthState({
          loading: false,
          authenticated: true,
          user: result.user,
          error: null
        });
      } else {
        setAuthState({
          loading: false,
          authenticated: false,
          user: null,
          error: result.error || 'Authentication failed'
        });
      }
    } catch (error) {
      const errorMessage = error instanceof Error ? error.message : 'Authentication failed';
      
      setAuthState({
        loading: false,
        authenticated: false,
        user: null,
        error: errorMessage
      });
    }
  };

  const handleLogout = (): void => {
    TokenManager.clearToken();
    window.close();
  };

  useEffect(() => {
    initializeAuth();
  }, []);

  // Render based on auth state
  if (authState.loading) {
    return <LoadingScreen />;
  }

  if (!authState.authenticated || !authState.user || authState.error) {
    return (
      <ErrorScreen 
        error={authState.error || 'Authentication required'} 
        onRetry={initializeAuth}
      />
    );
  }

  return (
    <div className="chat-app">
      <ChatInterface 
        user={authState.user} 
        onLogout={handleLogout} 
      />
    </div>
  );
};

export default App;