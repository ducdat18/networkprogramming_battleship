# 2. Introduction

## 2.1 Project Overview

**Battleship Multiplayer** is a networked implementation of the classic naval warfare board game, developed as a comprehensive Network Programming project. The application demonstrates client-server architecture, real-time multiplayer gameplay, secure authentication, and database persistence using modern C++ technologies.

### Game Description

Battleship is a two-player strategy game where each player secretly arranges five ships of different sizes on a 10×10 grid. Players take turns calling out coordinates to "fire" at opponent's ships. The objective is to sink all of the opponent's ships before they sink yours.

**Ship Types:**
- Carrier (5 cells)
- Battleship (4 cells)
- Cruiser (3 cells)
- Submarine (3 cells)
- Destroyer (2 cells)

**Game Rules:**
- Each ship occupies consecutive cells (horizontal or vertical)
- Ships cannot overlap or touch each other
- Players alternate turns firing at grid coordinates
- Hit/Miss/Sunk feedback is provided after each shot
- First player to sink all opponent's ships wins

---

## 2.2 Project Objectives

### Primary Goals

1. **Network Programming Fundamentals**
   - Implement reliable TCP client-server communication
   - Design efficient binary protocol for game messages
   - Handle concurrent multiple client connections
   - Implement real-time bidirectional messaging

2. **Multiplayer Game Architecture**
   - Server-authoritative game state management
   - Client-side input and rendering
   - Turn-based synchronization between players
   - Real-time player presence and status updates

3. **Security & Authentication**
   - Secure user registration and login system
   - Password hashing using SHA-256 with salt
   - Session-based authentication with token validation
   - Protection against common vulnerabilities (SQL injection, timing attacks)

4. **Data Persistence**
   - Relational database design (SQLite)
   - User account management
   - Match history and move logging
   - ELO rating system for competitive ranking

5. **User Experience**
   - Responsive GTK-based graphical user interface
   - Intuitive lobby and matchmaking system
   - Real-time game updates and animations
   - Auto-login for seamless user experience

### Learning Outcomes

- Master socket programming in C++ (BSD sockets)
- Understand client-server architecture patterns
- Learn protocol design and message serialization
- Implement concurrent server handling multiple clients
- Practice database integration and SQL
- Apply cryptographic techniques for security
- Develop real-time multiplayer game synchronization

---

## 2.3 Technology Stack

### Core Technologies

| Component | Technology | Purpose |
|-----------|-----------|---------|
| **Language** | C++17 | Core application logic |
| **GUI Framework** | GTK+ 3.0 | Client user interface |
| **Database** | SQLite 3 | Data persistence |
| **Cryptography** | OpenSSL (libssl, libcrypto) | Password hashing (SHA-256) |
| **Networking** | BSD Sockets (POSIX) | TCP/IP communication |
| **Testing** | Google Test | Unit and integration tests |
| **Build System** | GNU Make | Compilation and linking |

### Development Tools

- **Version Control**: Git
- **Compiler**: g++ (GCC 9.0+)
- **Debugger**: GDB, Valgrind (memory leak detection)
- **Documentation**: Markdown (CLAUDE.md)
- **Containerization**: Docker + Docker Compose (deployment)

---

## 2.4 System Architecture Overview

### High-Level Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                         CLIENT                               │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │  UI Manager  │  │ Client       │  │   Session    │      │
│  │   (GTK)      │◄─┤  Network     │◄─┤   Storage    │      │
│  └──────────────┘  └──────────────┘  └──────────────┘      │
│         │                  │                                 │
│         └──────────────────┼─────────────────────────────────┤
│                            │  TCP Socket                     │
└────────────────────────────┼─────────────────────────────────┘
                             │
                             │  Protocol Messages
                             │  (Binary Format)
                             │
┌────────────────────────────┼─────────────────────────────────┐
│                            │  TCP Socket                     │
│  ┌──────────────┬──────────┴──────────┬──────────────┐      │
│  │   Server     │  Message Handlers   │   Managers   │      │
│  │  (Listener)  │  - AuthHandler      │  - Player    │      │
│  │              │  - PlayerHandler    │  - Challenge │      │
│  │              │  - ChallengeHandler │  - Database  │      │
│  └──────────────┴─────────────────────┴──────────────┘      │
│                         SERVER                               │
└─────────────────────────────────────────────────────────────┘
                             │
                             ▼
                    ┌─────────────────┐
                    │  SQLite Database│
                    │  - users        │
                    │  - sessions     │
                    │  - matches      │
                    │  - match_boards │
                    │  - match_moves  │
                    └─────────────────┘
```

### Communication Flow

1. **Client → Server**: User actions (login, challenge, move) sent as binary messages
2. **Server Processing**: Validates request, updates database, processes game logic
3. **Server → Client(s)**: Broadcasts results to relevant clients
4. **Database**: Persists all user data, sessions, and game state

---

## 2.5 Key Features Implemented

### ✅ Phase 1: Networking Foundation
- TCP client-server connection with proper error handling
- Binary protocol with message types (40+ message types)
- Multi-threaded server handling concurrent clients
- Message serialization and deserialization
- Connection lifecycle management

### ✅ Phase 2: Authentication System
- User registration with unique username validation
- Secure login with SHA-256 password hashing + random salt
- Session management with 24-hour expiration
- Auto-login feature with client-side session storage
- Server-side session validation on reconnect
- Database integration with SQLite (WAL mode for concurrency)

### ✅ Phase 3: Matchmaking & Lobby
- Real-time player list with online/offline status
- Direct challenge system (player-to-player)
- Challenge accept/decline with 60-second timeout
- Match creation and player status updates
- Broadcast notifications to all connected clients
- GTK-based lobby UI with TreeView

### 🚧 Phase 4: Core Gameplay (In Progress)
- Ship placement validation
- Turn-based move handling
- Hit/Miss/Sunk detection
- Win condition checking
- Game board UI with animations

---

## 2.6 Project Scope

### In Scope
✅ Client-server TCP networking
✅ User authentication and session management
✅ Player matchmaking (direct challenge)
✅ Turn-based gameplay synchronization
✅ Database persistence (users, sessions, matches)
✅ ELO rating system
✅ Match replay functionality

### Out of Scope
❌ AI opponent (human vs human only)
❌ Chat messaging between players
❌ Spectator mode
❌ Audio/voice chat
❌ Mobile client support
❌ Web-based interface

---

## 2.7 Project Significance

This project demonstrates practical application of network programming concepts:

- **Concurrent Programming**: Multi-threaded server handling multiple simultaneous connections
- **Protocol Design**: Efficient binary message format for low-latency communication
- **State Synchronization**: Ensuring consistent game state across distributed clients
- **Security**: Implementing industry-standard authentication and cryptography
- **Scalability**: Architecture supports 50+ concurrent players (stress tested)

The implementation follows software engineering best practices including:
- Modular architecture with clear separation of concerns
- Comprehensive test coverage (130+ unit and integration tests)
- Documentation-driven development
- Version control and CI/CD practices

---

## 2.8 Report Structure

This report is organized as follows:

1. **Team Division**: Work distribution and responsibilities
2. **Introduction** (this section): Project overview and objectives
3. **Design Diagrams**: Visual architecture and flow diagrams
4. **Packet Structure**: Protocol specification
5. **Application Features**: Implementation details and screenshots
6. **Conclusion**: Summary and future work

---

**Target Achievement**:
- **Base Requirements**: 11/11 points (100%)
- **Bonus Features**: +2-3 points (password hashing, auto-login, comprehensive testing)
- **Estimated Total**: 13-14 points

**Current Status**: Phase 3 complete (75%), Phase 4 in progress (25%)
