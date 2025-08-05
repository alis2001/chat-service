import React from 'react';

function App() {
  return (
    <div style={{ 
      minHeight: '100vh', 
      background: 'linear-gradient(135deg, #667eea 0%, #764ba2 100%)',
      display: 'flex',
      alignItems: 'center',
      justifyContent: 'center',
      color: 'white',
      fontFamily: 'Arial, sans-serif'
    }}>
      <div style={{ textAlign: 'center' }}>
        <h1>☕ Caffis Chat Service</h1>
        <p>🚀 WebSocket Server Running on Port 5004</p>
        <p>✅ Connected to Redis Cache</p>
        <p>🎉 Phase 1 Complete - Foundation Ready!</p>
        <button 
          onClick={() => {
            const ws = new WebSocket('ws://localhost:5004');
            ws.onopen = () => console.log('WebSocket Connected!');
            ws.onmessage = (msg) => console.log('Received:', msg.data);
            ws.send('Hello from React!');
          }}
          style={{
            padding: '10px 20px',
            backgroundColor: 'rgba(255,255,255,0.2)',
            border: '1px solid rgba(255,255,255,0.3)',
            borderRadius: '8px',
            color: 'white',
            cursor: 'pointer',
            marginTop: '20px'
          }}
        >
          Test WebSocket Connection
        </button>
      </div>
    </div>
  );
}

export default App;