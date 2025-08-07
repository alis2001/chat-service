import React, { useState, useEffect } from 'react';
import { MessageCircle, Coffee } from 'lucide-react';

interface LoadingScreenProps {
  message?: string;
}

const LoadingScreen: React.FC<LoadingScreenProps> = ({ 
  message = "Loading Caffis Chat..." 
}) => {
  const [currentDot, setCurrentDot] = useState(0);
  const [progress, setProgress] = useState(0);

  // Animated dots
  useEffect(() => {
    const interval = setInterval(() => {
      setCurrentDot(prev => (prev + 1) % 3);
    }, 500);
    return () => clearInterval(interval);
  }, []);

  // Smooth progress animation
  useEffect(() => {
    const progressInterval = setInterval(() => {
      setProgress(prev => {
        if (prev >= 95) return 95; // Stop at 95% until real loading completes
        return prev + Math.random() * 15;
      });
    }, 200);
    return () => clearInterval(progressInterval);
  }, []);

  return (
    <div className="chat-loading-screen">
      <div className="chat-loading-container">
        
        {/* Animated Icon */}
        <div className="chat-loading-icon-container">
          <div className="chat-loading-icon">
            <MessageCircle size={48} />
          </div>
          <div className="chat-loading-coffee">
            <Coffee size={24} />
          </div>
          <div className="chat-loading-pulse"></div>
        </div>
        
        {/* Loading Text */}
        <h2 className="chat-loading-title">
          {message}
        </h2>
        
        {/* Progress Bar */}
        <div className="chat-loading-progress">
          <div className="progress-track">
            <div 
              className="progress-fill"
              style={{ width: `${Math.min(progress, 100)}%` }}
            />
          </div>
        </div>
        
        {/* Animated Dots */}
        <div className="chat-loading-dots">
          {[0, 1, 2].map((index) => (
            <div 
              key={`loading-dot-${index}`} 
              className={`chat-loading-dot ${currentDot === index ? 'active' : ''}`}
            />
          ))}
        </div>

        {/* Floating Bubbles */}
        <div className="chat-loading-bubbles">
          {[...Array(6)].map((_, i) => (
            <div 
              key={`bubble-${i}`} 
              className="bubble" 
              style={{
                left: `${15 + i * 15}%`,
                animationDelay: `${i * 0.5}s`
              }}
            />
          ))}
        </div>
      </div>
    </div>
  );
};

export default LoadingScreen;