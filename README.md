# ☕ Caffis Chat Service

Enterprise-grade real-time messaging microservice for the Caffis ecosystem. Built with C++ WebSocket server, React frontend, and Redis caching.

![Caffis Chat](https://img.shields.io/badge/Status-Phase%201%20Complete-success)
![C++](https://img.shields.io/badge/C++-17-blue)
![React](https://img.shields.io/badge/React-18-blue)
![Docker](https://img.shields.io/badge/Docker-Compose-blue)

## 🎯 Project Overview

The Caffis Chat Service provides WhatsApp/Telegram-level messaging functionality specifically designed for coffee meetups and social connections. It integrates seamlessly with the main Caffis app and Map Service.

## 🏗️ Architecture
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   MAIN APP      │    │   MAP SERVICE   │    │  CHAT SERVICE   │
│   Port: 3000    │    │   Port: 3001    │    │   Port: 3003    │
│   Backend: 5000 │    │   Backend: 5001 │    │   Backend: 5004 │
└─────────────────┘    └─────────────────┘    └─────────────────┘
│
┌─────────────────────┐
│   POSTGRESQL DB     │
│ + Redis Cluster     │
│ Shared & Extended   │
└─────────────────────┘


## 🛠️ Tech Stack

**Backend (C++):**
- C++17 with Boost.Beast WebSocket library
- PostgreSQL with libpqxx driver
- Redis for message caching and pub/sub
- Docker containerization

**Frontend (React):**
- React 18 with TypeScript
- WebSocket client for real-time messaging
- Framer Motion animations
- Apple-inspired design system

**Infrastructure:**
- Docker Compose orchestration
- Redis 7 Alpine
- Ubuntu 22.04 base images
- CMake build system

## 🚀 Quick Start

### Prerequisites
- Docker and Docker Compose
- Git

### Installation

1. **Clone the repository:**
   ```bash
   git clone https://github.com/alis2001/chat-service.git
   cd chat-service
