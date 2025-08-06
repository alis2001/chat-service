import React, { useState, useRef } from 'react';
import { Send } from 'lucide-react';

interface MessageSendAnimationProps {
  onSendMessage: (content: string) => void;
  disabled?: boolean;
}

const MessageSendAnimation: React.FC<MessageSendAnimationProps> = ({ 
  onSendMessage, 
  disabled = false 
}) => {
  const [message, setMessage] = useState('');
  const [isSending, setIsSending] = useState(false);
  const inputRef = useRef<HTMLTextAreaElement>(null);
  const sendButtonRef = useRef<HTMLButtonElement>(null);

  const handleSend = async () => {
    if (!message.trim() || disabled || isSending) return;

    setIsSending(true);

    // Create beautiful send animation
    const sendButton = sendButtonRef.current;
    const messageInput = inputRef.current;

    if (sendButton && messageInput) {
      // Add liquid pop effect to button
      sendButton.style.transform = 'scale(0.8) rotate(20deg)';
      sendButton.style.transition = 'all 0.2s cubic-bezier(0.68, -0.6, 0.32, 1.6)';
      
      // Create message bubble that flies out
      const messageBubble = document.createElement('div');
      messageBubble.textContent = message;
      messageBubble.style.cssText = `
        position: fixed;
        background: linear-gradient(135deg, #FF6B6B, #FF8E8E);
        color: white;
        padding: 12px 16px;
        border-radius: 18px;
        font-size: 14px;
        font-weight: 500;
        pointer-events: none;
        z-index: 1000;
        box-shadow: 0 4px 20px rgba(255, 107, 107, 0.4);
        max-width: 200px;
        word-wrap: break-word;
      `;

      // Position bubble at input location
      const inputRect = messageInput.getBoundingClientRect();
      messageBubble.style.left = `${inputRect.left}px`;
      messageBubble.style.top = `${inputRect.top}px`;
      
      document.body.appendChild(messageBubble);

      // Animate bubble flying up and scaling down
      messageBubble.animate([
        { 
          transform: 'translateY(0) scale(1)', 
          opacity: 1 
        },
        { 
          transform: 'translateY(-100px) scale(0.3)', 
          opacity: 0 
        }
      ], {
        duration: 800,
        easing: 'cubic-bezier(0.68, -0.55, 0.265, 1.55)'
      });

      // Clean up bubble after animation
      setTimeout(() => {
        document.body.removeChild(messageBubble);
      }, 800);

      // Reset button after animation
      setTimeout(() => {
        sendButton.style.transform = 'scale(1) rotate(0deg)';
      }, 300);
    }

    // Send the actual message
    try {
      await onSendMessage(message);
      
      // Clear input with beautiful animation
      if (messageInput) {
        messageInput.style.transform = 'scale(0.95)';
        messageInput.style.transition = 'all 0.2s ease-out';
        
        setTimeout(() => {
          setMessage('');
          messageInput.style.transform = 'scale(1)';
        }, 100);
      }
    } catch (error) {
      console.error('Send failed:', error);
    }

    setIsSending(false);
  };

  const handleKeyPress = (e: React.KeyboardEvent) => {
    if (e.key === 'Enter' && !e.shiftKey) {
      e.preventDefault();
      handleSend();
    }
  };

  return (
    <div className="message-input-wrapper">
      <textarea
        ref={inputRef}
        value={message}
        onChange={(e) => setMessage(e.target.value)}
        onKeyPress={handleKeyPress}
        placeholder="Type your message..."
        className="message-input"
        rows={1}
        disabled={disabled || isSending}
        style={{
          opacity: isSending ? 0.7 : 1,
          transition: 'opacity 0.3s ease'
        }}
      />
      
      <button 
        ref={sendButtonRef}
        className="send-button"
        onClick={handleSend}
        disabled={!message.trim() || disabled || isSending}
        title="Send Message"
        style={{
          opacity: isSending ? 0.8 : 1,
        }}
      >
        <Send size={20} />
      </button>
      
      {/* Beautiful typing indicator when sending */}
      {isSending && (
        <div className="typing-indicator" style={{ 
          position: 'absolute', 
          bottom: '100%', 
          left: '50%', 
          transform: 'translateX(-50%)',
          marginBottom: '8px'
        }}>
          <div className="typing-dot"></div>
          <div className="typing-dot"></div>
          <div className="typing-dot"></div>
        </div>
      )}
    </div>
  );
};

export default MessageSendAnimation;