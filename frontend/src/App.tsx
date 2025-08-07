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

  // Add state for delayed loading display
  const [showLoading, setShowLoading] = useState(false);

  const initializeAuth = async (): Promise<void> => {
    setAuthState(prev => ({ 
      ...prev, 
      loading: true, 
      error: null 
    }));

    // Only show loading if it takes longer than 300ms
    const loadingTimer = setTimeout(() => {
      setShowLoading(true);
    }, 300);
    
    try {
      const result = await ChatAuth.initialize();
      
      // Clear timer and hide loading
      clearTimeout(loadingTimer);
      
      if (result.success && result.user) {
        setAuthState({
          loading: false,
          authenticated: true,
          user: result.user,
          error: null
        });
        setShowLoading(false);
      } else {
        setAuthState({
          loading: false,
          authenticated: false,
          user: null,
          error: result.error || 'Authentication failed'
        });
        setShowLoading(false);
      }
    } catch (error) {
      clearTimeout(loadingTimer);
      setShowLoading(false);
      
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

  // Show loading only if needed and delayed
  if (authState.loading && showLoading) {
    return <LoadingScreen message="Loading Caffis Chat..." />;
  }

  // Show error screen
  if (!authState.authenticated || !authState.user || authState.error) {
    return (
      <ErrorScreen 
        error={authState.error || 'Authentication required'} 
        onRetry={initializeAuth}
      />
    );
  }

  // Show chat interface
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