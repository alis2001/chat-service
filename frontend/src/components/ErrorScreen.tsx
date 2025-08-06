import React from 'react';
import { WifiOff } from 'lucide-react';

interface ErrorScreenProps {
  error: string;
  onRetry: () => void;
}

const ErrorScreen: React.FC<ErrorScreenProps> = ({ error, onRetry }) => {
  return (
    <div className="error-screen">
      <div className="error-container">
        <div className="error-icon">
          <WifiOff size={40} />
        </div>
        
        <h2 className="error-title">
          Connection Failed
        </h2>
        
        <p className="error-message">
          {error || "Unable to authenticate with the main Caffis app"}
        </p>
        
        <button
          className="error-retry-button"
          onClick={onRetry}
          type="button"
        >
          Try Again
        </button>
        
        <p className="error-help-text">
          Make sure you're accessing chat from the main Caffis app
        </p>
      </div>
    </div>
  );
};

export default ErrorScreen;