import React from 'react';
import { MessageCircle } from 'lucide-react';

interface LoadingScreenProps {
  message?: string;
}

const LoadingScreen: React.FC<LoadingScreenProps> = ({ 
  message = "Connecting to Caffis Chat..." 
}) => {
  return (
    <div className="loading-screen">
      <div className="loading-container">
        <div className="loading-icon">
          <MessageCircle size={48} />
        </div>
        
        <h2 className="loading-title">
          {message}
        </h2>
        
        <div className="loading-dots">
          {[0, 1, 2].map((index) => (
            <div 
              key={`loading-dot-${index}`} 
              className="loading-dot" 
            />
          ))}
        </div>
      </div>
    </div>
  );
};

export default LoadingScreen;