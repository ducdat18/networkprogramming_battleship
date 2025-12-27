# Battleship Multiplayer - Development Notes

## Current Phase: 3 - Matchmaking & Lobby (IN PROGRESS) 🚀

### Phase 3 Timeline: 8-9 days - 5-6 points target

**Goal**: Enable players to find each other, send challenges, and start matches

**Two Matchmaking Modes:**
1. **Direct Challenge** - Challenge specific player (required - 5 điểm) ✅ COMPLETE
2. **Auto-Matchmaking** - Queue-based ELO matching (bonus - +0.5-1 điểm)

---

## ✅ Phase 3.2 - Challenge System (COMPLETE - Days 3-4)

**Server-Side Challenge Flow Implementation:**
- ✅ Created `ChallengeManager` class for challenge lifecycle management
- ✅ Created `ChallengeHandler` for routing challenge messages
- ✅ Challenge sending with validation (self-challenge, offline, in-game checks)
- ✅ Challenge routing to target player
- ✅ Accept/Decline challenge responses
- ✅ Match creation on acceptance with database integration
- ✅ Challenge timeout handling (60 seconds)
- ✅ Thread-safe concurrent operations
- ✅ Player status updates (AVAILABLE → IN_GAME)
- ✅ 11 comprehensive unit tests - ALL PASSING ✅
- ✅ 4 integration tests for end-to-end challenge flow - ALL PASSING ✅

**Protocol Messages Implemented:**
```cpp
CHALLENGE_SEND = 20,        // Send challenge to another player
CHALLENGE_RECEIVED = 21,    // Notify target of incoming challenge
CHALLENGE_RESPONSE = 22,    // Accept/decline challenge
MATCH_START = 23,          // Notify both players match is created
```

**Challenge Flow:**
```
Player 1                    Server                    Player 2
   |                          |                          |
   |--- CHALLENGE_SEND -----> |                          |
   |  (target_user_id=2)      | [Validate & Store]       |
   |                          |--- CHALLENGE_RECEIVED -->|
   |                          |  (challenge_id, info)    |
   |                          |                          |
   |                          | <-- CHALLENGE_RESPONSE --|
   |                          |  (challenge_id, accept)  |
   |                          | [Create Match in DB]     |
   | <-- MATCH_START -------- |                          |
   |  (match_id=1)            |-------- MATCH_START ---->|
   |                          |  (match_id=1)            |
```

**Validation Logic:**
- Cannot challenge yourself
- Cannot challenge offline players
- Cannot challenge players already in a game (STATUS_IN_GAME)
- Cannot respond to non-existent challenges
- Only the target can respond to a challenge
- Challenge expires after 60 seconds

**Files Created (6):**
1. `server/include/challenge_manager.h` - Challenge lifecycle management
2. `server/src/challenge_manager.cpp` - Implementation with timeout handling
3. `server/include/challenge_handler.h` - CHALLENGE_* message handler
4. `server/src/challenge_handler.cpp` - Challenge routing logic
5. `tests/unit_tests/server/test_challenge_manager.cpp` - 11 unit tests
6. `tests/integration_tests/test_challenge.cpp` - 4 integration tests
7. `tests/manual/test_challenge_manual.cpp` - Manual E2E test

**Files Modified (4):**
1. `server/include/server.h` - Added ChallengeManager member
2. `server/src/server.cpp` - Integrated ChallengeManager, setup handlers
3. `Makefile` - Added TEST_CHALLENGE_MANAGER and TEST_CHALLENGE targets
4. `run_integration_tests.sh` - Added challenge integration test + 0.5s delays between suites

**Testing Status:**
- **Unit Tests**: 11/11 tests PASSING ✅
  - SendChallenge_Valid
  - SendChallenge_CannotChallengeSelf
  - SendChallenge_TargetOffline
  - SendChallenge_TargetInGame
  - AcceptChallenge_CreatesMatch
  - DeclineChallenge
  - RespondToNonexistentChallenge
  - WrongUserCannotRespond
  - MultiplePendingChallenges
  - RemovePlayerChallenges
  - CancelChallenge

- **Integration Tests**: 4/4 tests PASSING ✅
  - SendAndReceiveChallenge
  - AcceptChallengeCreatesMatch
  - DeclineChallengeNoMatch
  - MultipleChallengesToSameUser

**Critical Bug Fixed - MessageHeader Initialization:**
- **Issue**: "Bad address" errors when sending challenge notifications
- **Root Cause**: MessageHeader not properly initialized (missing memset and field values)
- **Fix**: Established proper initialization pattern:
```cpp
MessageHeader header;
memset(&header, 0, sizeof(header));
header.type = CHALLENGE_RECEIVED;
header.length = sizeof(msg);
header.timestamp = time(nullptr);
```
- **Verification**: Manual test and all integration tests pass

**Critical Bug Fixed - Integration Test Timing:**
- **Issue**: Tests pass standalone but fail intermittently in full suite
- **Root Cause**: All tests share single server instance without cleanup time
- **Fix**: Added 0.5s delays between test suites in run_integration_tests.sh
- **Additional Fixes**:
  - Increased socket recv timeout: 5s → 10s
  - Added 300ms post-login delay for PlayerManager registration
  - Increased max_attempts: 10 → 20 for broadcast message skipping
  - Added 200ms TearDown delay for connection cleanup
- **Result**: All 36 integration tests now pass reliably ✅

**Architecture:**
```
Server
  ├── PlayerManager (Phase 3.1)
  │    ├── Track online players
  │    └── Broadcast status updates
  │
  └── ChallengeManager (Phase 3.2) ⭐ NEW
       ├── Track pending challenges (map<challenge_id, PendingChallenge>)
       ├── Challenge ID generation (auto-increment)
       ├── Validate challenge requests
       ├── Route challenges to targets
       ├── Handle accept/decline responses
       ├── Create matches in database
       ├── Update player status to IN_GAME
       ├── Timeout tracking (60 seconds)
       └── Thread-safe operations (std::mutex)

Challenge Lifecycle:
1. User 1 sends CHALLENGE_SEND → Server validates
2. Server stores in pending challenges map
3. Server sends CHALLENGE_RECEIVED to User 2
4. User 2 sends CHALLENGE_RESPONSE (accept/decline)
5. If accept: Server creates match, updates status, sends MATCH_START to both
6. If decline: Challenge removed, players stay AVAILABLE
7. If timeout: Challenge auto-removed after 60s
```

**Deliverable 3.2**: ✅ Server can route challenges, handle responses, and create matches

---

## ✅ Phase 3.1 - Player List Server (COMPLETE - Days 1-2)

**Server-Side Player Tracking Implementation:**
- ✅ Created `PlayerManager` class for tracking online players
- ✅ Track player status (available/in_game/busy/offline)
- ✅ Real-time status broadcasting to all clients
- ✅ Created `PlayerHandler` for PLAYER_LIST_REQUEST messages
- ✅ Server integration with broadcast capability
- ✅ AuthHandler integration (register/remove players on login/logout)
- ✅ Thread-safe concurrent operations
- ✅ 13 comprehensive unit tests - ALL PASSING ✅
- ✅ Integration tests for end-to-end player list flow

**Critical Bug Found & Fixed:**
- **Issue**: Deadlock in PlayerManager causing unit tests to hang
- **Root Cause**:
  - `addPlayer()`, `removePlayer()`, `updatePlayerStatus()` held `mutex_` lock
  - Called `broadcastPlayerStatusUpdate()` while holding lock
  - `broadcastPlayerStatusUpdate()` also needs `mutex_` → **DEADLOCK**
- **Fix**: Release lock before broadcasting using scope blocks `{}`
- **Verification**: All 13 unit tests pass including concurrent operations with 100 players

**Files Created (6):**
1. `server/include/player_manager.h` - Player tracking manager
2. `server/src/player_manager.cpp` - Implementation with thread safety
3. `server/include/player_handler.h` - PLAYER_LIST_REQUEST handler
4. `server/src/player_handler.cpp` - Player list response logic
5. `tests/unit_tests/server/test_player_manager.cpp` - 13 unit tests
6. `tests/integration_tests/test_player_list.cpp` - Integration tests

**Files Modified (7):**
1. `server/include/server.h` - Added PlayerManager + broadcast() method
2. `server/src/server.cpp` - Integrated PlayerManager, setup handlers
3. `server/include/auth_handler.h` - Added server reference for PlayerManager
4. `server/src/auth_handler.cpp` - Register/remove players on login/logout/validate
5. `Makefile` - Added TEST_PLAYER_MANAGER and TEST_PLAYER_LIST targets
6. `run_integration_tests.sh` - Added player list integration test

**Testing Status:**
- **Unit Tests**: 13/13 tests PASSING ✅
  - AddPlayer, AddMultiplePlayers
  - RemovePlayer
  - UpdatePlayerStatus
  - GetPlayerInfo, GetNonExistentPlayerInfo
  - GetOnlinePlayers, GetAvailablePlayers
  - EmptyPlayerList
  - PlayerStatusTransitions
  - **ConcurrentAddRemove** (100 players) ✅
  - **ConcurrentStatusUpdates** (50 players) ✅
  - GetClientConnection

- **Integration Tests**: Built successfully ✅
  - LoginRegistersPlayer
  - RequestPlayerListReturnsPlayers
  - MultipleClientsSeeEachOther
  - LogoutRemovesPlayer
  - PlayerListContainsCorrectInfo

**Architecture:**
```
Server
  └── PlayerManager
       ├── Track online players (std::map<user_id, PlayerData>)
       ├── Thread-safe operations (std::mutex)
       ├── Broadcast status updates (via Server::broadcast())
       └── Filter available/in_game players

Client → PLAYER_LIST_REQUEST → Server
Server → PlayerHandler → PlayerManager::getOnlinePlayers()
Server → PLAYER_LIST (PlayerListResponse) → Client

On Login/Logout:
AuthHandler → PlayerManager::addPlayer() / removePlayer()
PlayerManager → broadcastPlayerStatusUpdate() → All Clients
```

**Deliverable 3.1**: ✅ Server can track online players and broadcast status updates

---

## ✅ Phase 3.1.1 - Config Consolidation & Test Stability (COMPLETE)

**Configuration Consolidation:**
- ✅ Created unified `common/include/config.h` with all shared settings:
  - Network: SERVER_HOST, SERVER_PORT, MAX_CLIENTS, BUFFER_SIZE
  - Database: DATABASE_PATH
  - Application: APP_VERSION
  - Client UI: WINDOW_TITLE, WINDOW_WIDTH, WINDOW_HEIGHT
- ✅ Removed redundant `client/include/config.h` (was including server config)
- ✅ Removed `server/include/config.h` (moved to common)
- ✅ All components now use single config source

**Integration Test Fixes:**
- ✅ Fixed port mismatch: tests were using port 8888, server uses 9999
- ✅ All test files now use `SERVER_PORT` from `common/include/config.h`
- ✅ Fixed unique username generation to prevent test collisions:
  - Added random component: `username + timestamp + "_" + rand()`
  - Added `srand()` seeding in test setup
- ✅ Fixed logout test: Server now looks up user_id from session_token
- ✅ Added `receiveExpectedMessage()` to skip broadcast messages during tests

**Bug Fixes:**
- **Logout not removing player**: Server's `handleLogout()` now uses `db_->validateSession()` to get user_id from session token instead of relying on client object
- **Test flakiness**: Tests now properly skip `PLAYER_STATUS_UPDATE` broadcasts when waiting for specific responses

**Files Modified:**
- `common/include/config.h` - Unified configuration (created/updated)
- `server/src/auth_handler.cpp` - Fixed logout to use session-based user_id lookup
- `tests/integration_tests/test_player_list.cpp` - Added random usernames, skip broadcasts
- `tests/integration_tests/test_authentication.cpp` - Use config, unique usernames
- `tests/integration_tests/test_client_server.cpp` - Use config
- `tests/integration_tests/test_e2e_client_auth.cpp` - Use config, unique usernames
- `tests/integration_tests/test_auto_login.cpp` - Use config

**Files Removed:**
- `client/include/config.h` - Redundant, now uses common config
- `server/include/config.h` - Moved to common

**Test Results: ALL 32 INTEGRATION TESTS PASSING** ✅
```
Client-Server:    9 tests ✅
Authentication:   7 tests ✅
E2E Client Auth:  5 tests ✅
Auto-Login:       6 tests ✅
Player List:      5 tests ✅
```

---

## Previous Phases

---

## ✅ Phase 2.1 - Database Setup (COMPLETE)

**Database Implementation:**
- ✅ Created DatabaseManager class with SQLite integration
- ✅ Defined comprehensive database schema:
  - `users` table (user_id, username, password_hash, display_name, elo_rating, created_at, last_login)
  - `sessions` table (session_id, user_id, session_token, created_at, expires_at)
  - `matches` table (match_id, player1_id, player2_id, winner_id, status, created_at, ended_at)
  - `match_boards` table (board_id, match_id, user_id, ship_data)
  - `match_moves` table (move_id, match_id, player_id, move_number, x, y, result, timestamp)
- ✅ Implemented CRUD operations for all tables
- ✅ Migrated AuthHandler from in-memory to database
- ✅ Added session expiration logic (24h default)
- ✅ Enabled WAL (Write-Ahead Logging) mode for concurrent access
- ✅ Database file: `data/battleship.db`

---

## ✅ Phase 2.2 - Password Hashing (COMPLETE - BONUS POINTS!)

**SHA-256 Password Hashing Implementation:**
- ✅ Created `PasswordHash` class with SHA-256 + random salt
- ✅ Hash format: `salt$hash` (32 hex chars salt + 64 hex chars hash)
- ✅ Constant-time password comparison (timing attack prevention)
- ✅ Integrated into AuthHandler for register/login
- ✅ 16 comprehensive unit tests

**Security Features:**
- Random salt generation (16 bytes)
- SHA-256 cryptographic hash
- Constant-time comparison to prevent timing attacks
- Secure password validation

**Files:**
- `common/include/password_hash.h` - Password hashing header
- `common/src/password_hash.cpp` - Implementation with OpenSSL
- `server/src/auth_handler.cpp` - Updated to use password hashing
- `tests/unit_tests/crypto/test_password_hash.cpp` - 16 tests

---

## ✅ Phase 2.3 - Auto-Login (COMPLETE - BETTER UX!)

**Session Persistence Implementation:**
- ✅ Created `SessionStorage` class for client-side session management
- ✅ Session file: `~/.battleship/session.txt`
- ✅ Secure file permissions: 0600 (owner read/write only)
- ✅ Stores: user_id, session_token, username, display_name, elo_rating
- ✅ Integrated into UIManager:
  - Save session on successful login
  - Clear session on logout
  - Auto-restore session on app startup
- ✅ 8 unit tests for SessionStorage
- ✅ 6 integration tests for auto-login flow

**⚙️ Auto-Login Toggle Feature:**
Auto-login có thể bật/tắt qua config để thuận tiện cho việc demo trên 1 máy với nhiều clients:

```cpp
// common/include/config.h
#define AUTO_LOGIN_ENABLED 0  // 0 = tắt, 1 = bật
```

**Khi tắt (AUTO_LOGIN_ENABLED = 0):**
- Mỗi client khởi động sẽ hiển thị màn hình login
- Session không được lưu vào file
- Phù hợp cho demo với nhiều accounts trên 1 máy

**Khi bật (AUTO_LOGIN_ENABLED = 1):**
- Session được lưu vào `~/.battleship/session.txt`
- Lần sau mở app sẽ tự động login (nếu session hợp lệ)
- Phù hợp cho sử dụng thực tế

**Files:**
- `client/include/session_storage.h` - Session storage header
- `client/src/session_storage.cpp` - File-based session persistence
- `client/src/ui_manager.cpp` - Auto-login integration (sử dụng `AUTO_LOGIN_ENABLED`)
- `common/include/config.h` - Cấu hình `AUTO_LOGIN_ENABLED`
- `tests/unit_tests/client/test_session_storage.cpp` - 8 tests
- `tests/integration_tests/test_auto_login.cpp` - 6 integration tests

---

## ✅ Phase 2.4 - Database Unit Tests (COMPLETE - MORE COVERAGE!)

**Comprehensive DatabaseManager Testing:**
- ✅ 37 unit tests covering all database operations
- ✅ User CRUD operations (11 tests)
- ✅ Session management (11 tests)
- ✅ Match operations (5 tests)
- ✅ Board & move operations (4 tests)
- ✅ Error handling & stress tests (6 tests)

**Test Coverage:**
- Create/get/update user operations
- Session creation, validation, expiration
- Match lifecycle management
- Ship placement and move logging
- Concurrent user creation (100 users)
- Token uniqueness constraints
- Edge cases (empty username, invalid IDs, etc.)

**Files:**
- `tests/unit_tests/database/test_database.cpp` - 37 comprehensive tests

---

## ✅ Phase 2.5 - Session Validation with Server (COMPLETE - SECURITY!)

**Server-Side Token Validation Implementation:**
- ✅ Added `VALIDATE_SESSION` message type (MessageType = 5)
- ✅ Client-side `validateSession()` method in ClientNetwork
- ✅ Server-side `handleValidateSession()` in AuthHandler
- ✅ Integrated into auto-login flow (connect → validate → show UI)
- ✅ Created `client/include/config.h` for centralized configuration
- ✅ Fixed critical enum collision bug (PendingRequest vs MessageType)

**Security Enhancement:**
- Before: Auto-login showed UI immediately with stored token (no server check)
- After: Auto-login validates token with server before showing UI
- Prevents expired/invalid tokens from granting access
- Server validates token expiration and user existence

**Bug Fix - Enum Collision:**
- **Problem**: Two enums with same member name `VALIDATE_SESSION`
  - `MessageType::VALIDATE_SESSION = 5` (protocol.h)
  - `PendingRequest::VALIDATE_SESSION = 4` (client_network.h - auto-numbered)
- **Symptom**: Client sent type=4 instead of type=5
- **Fix**: Used explicit namespace `MessageType::VALIDATE_SESSION`
- **Lesson**: Always use qualified enum names when multiple enums exist

**Files Created:**
- `client/include/config.h` - Centralized client configuration (SERVER_HOST, SERVER_PORT)

**Files Modified:**
- `common/include/protocol.h` - Added VALIDATE_SESSION = 5 to MessageType
- `client/include/client_network.h` - Added validateSession() method
- `client/src/client_network.cpp` - Implemented validation logic (fixed enum bug)
- `client/src/ui_manager.cpp` - Updated auto-login to validate with server
- `server/include/auth_handler.h` - Added handleValidateSession() declaration
- `server/src/auth_handler.cpp` - Implemented server-side validation handler

**Flow:**
```
App Startup
  └── Load session from ~/.battleship/session.txt
       └── Connect to server
            └── Send VALIDATE_SESSION request
                 ├── Valid → Show main menu
                 └── Invalid → Clear session, show login screen
```

---

## 📊 Testing Status - ALL PASSING ✅

**Unit Tests: 94 tests (10 suites)** ✅
- Board Tests: 15 tests ✅
- Match Tests: 3 tests ✅
- Auth Messages: 14 tests ✅
- Network Tests: 6 tests ✅
- Client Network: 11 tests ✅
- Session Storage: 8 tests ✅
- Password Hash: 16 tests ✅
- Database Manager: 37 tests ✅
- Player Manager: 13 tests ✅
- Challenge Manager: 11 tests ✅ ⭐ NEW

**Integration Tests: 36 tests (6 suites)** ✅
- Client-Server: 9 tests ✅
- Authentication: 7 tests ✅
- E2E Client Auth: 5 tests ✅
- Auto-Login: 6 tests ✅
- Player List: 5 tests ✅
- Challenge System: 4 tests ✅ ⭐ NEW

**Total: 130 tests - ALL PASSING!** 🎉

**Recent Fixes (Phase 3.2):**
- ✅ Fixed MessageHeader initialization ("Bad address" error)
- ✅ Fixed integration test timing issues (added inter-suite delays)
- ✅ Increased socket recv timeout (5s → 10s)
- ✅ Added PlayerManager registration delay (300ms)
- ✅ Increased broadcast message skip attempts (10 → 20)
- ✅ All 130 tests now pass reliably ✅

---

## 🔧 Infrastructure Updates

**Makefile Updates:**
- ✅ Added OpenSSL linking for all tests (`-lssl -lcrypto`)
- ✅ Added new test targets:
  - `TEST_SESSION_STORAGE`
  - `TEST_PASSWORD_HASH`
  - `TEST_DATABASE`
  - `TEST_AUTO_LOGIN`
- ✅ Updated `test-unit` target to run all 8 test suites
- ✅ Client now links with OpenSSL (password hashing in common code)

**Test Runner Script Updates (`run_integration_tests.sh`):**
- ✅ Clean WAL files (`*.db-shm`, `*.db-wal`)
- ✅ Clean session files (`~/.battleship/session.txt`)
- ✅ Build auto-login integration test
- ✅ Run auto-login tests
- ✅ Enhanced test summary output
- ✅ Proper cleanup of WAL files on exit
- ✅ Docker support with `--docker` flag

**Docker Updates:**
- ✅ Added `libssl-dev` to build stage (OpenSSL development headers)
- ✅ Added `libssl3` to runtime stage (OpenSSL runtime libraries)
- ✅ Added `netcat-openbsd` for healthcheck
- ✅ Multi-stage build with OpenSSL support
- ✅ Volume persistence for database
- ✅ Full Docker Compose integration

---

## 📁 Files Created/Modified (Phase 2)

**New Files (11):**
1. `common/include/password_hash.h` - Password hashing header
2. `common/src/password_hash.cpp` - SHA-256 implementation
3. `client/include/session_storage.h` - Session persistence header
4. `client/src/session_storage.cpp` - File-based session storage
5. `common/include/config.h` - **Unified configuration** (SERVER_HOST, SERVER_PORT, UI settings) ⭐ CONSOLIDATED
6. `server/include/database.h` - DatabaseManager header
7. `server/src/database.cpp` - SQLite database implementation
8. `tests/unit_tests/client/test_session_storage.cpp` - 8 tests
9. `tests/unit_tests/crypto/test_password_hash.cpp` - 16 tests
10. `tests/unit_tests/database/test_database.cpp` - 37 tests
11. `tests/integration_tests/test_auto_login.cpp` - 6 integration tests

**Removed Files (Phase 3.1.1):**
1. `client/include/config.h` - Removed, consolidated into common/include/config.h
2. `server/include/config.h` - Removed, consolidated into common/include/config.h

**Modified Files (15):**
1. `common/include/protocol.h` - Added VALIDATE_SESSION message type ⭐ NEW
2. `server/include/auth_handler.h` - Password hashing + session validation ⭐ UPDATED
3. `server/src/auth_handler.cpp` - Database + password hash + validation handler + logout fix ⭐ UPDATED
4. `server/include/server.h` - DatabaseManager member
5. `server/src/server.cpp` - Database initialization with WAL mode
6. `client/include/client_network.h` - Added validateSession() method ⭐ NEW
7. `client/src/client_network.cpp` - Validation logic + enum bug fix ⭐ NEW
8. `client/src/ui_manager.cpp` - Auto-login with server validation ⭐ UPDATED
9. `Makefile` - OpenSSL linking, new test targets
10. `run_integration_tests.sh` - Enhanced cleanup, auto-login test
11. `Dockerfile` - OpenSSL dependencies
12. `docker-compose.yml` - Volume persistence (already configured)

**Total Lines Added:** ~2,800+ lines (production code + tests)
**Phase 2 Complete**: 5 sub-phases, 11 new files, 15 modified files

---

## 🏗️ Architecture

**Authentication Flow (Complete with Server Validation):**
```
Client (GTK UI)
  └── UIManager
       ├── Auto-login on startup
       │    └── Load from SessionStorage (~/.battleship/session.txt)
       │         └── Connect to server (config.h: SERVER_HOST, SERVER_PORT)
       │              └── Validate session with server ⭐ NEW
       │                   ├── Valid → Show main menu
       │                   └── Invalid → Clear session, show login
       ├── Register/Login via ClientNetwork
       └── Save session on success

ClientNetwork
  ├── Send auth requests (REGISTER, LOGIN, LOGOUT)
  └── Send VALIDATE_SESSION request ⭐ NEW

Server
  └── AuthHandler (handles MessageType::VALIDATE_SESSION) ⭐ UPDATED
       ├── Uses DatabaseManager for users/sessions
       ├── PasswordHash for secure password verification
       ├── Session token generation
       └── Session validation (checks expiration + user existence) ⭐ NEW

DatabaseManager (SQLite)
  ├── Users table (with password_hash)
  ├── Sessions table (24h expiration)
  └── WAL mode (concurrent access)

SessionStorage (Client-side)
  └── ~/.battleship/session.txt (0600 permissions)
```

**Security Layers:**
1. **Transport**: TCP sockets (can upgrade to TLS later)
2. **Passwords**: SHA-256 + random salt + constant-time comparison
3. **Sessions**: 24h expiration, server-side validation on every auto-login ⭐ ENHANCED
4. **File Security**: Session file with 0600 permissions
5. **Database**: Prepared statements (SQL injection prevention)
6. **Token Validation**: Server checks token validity before granting access ⭐ NEW

---

## 🚀 Quick Commands

```bash
# Build everything
make clean && make all

# Run all unit tests
make test-unit

# Run specific tests
./bin/test_password_hash
./bin/test_session_storage
./bin/test_database

# Run integration tests
./run_integration_tests.sh

# Run with Docker
./run_integration_tests.sh --docker

# ⚙️ Toggle Auto-Login (cho demo trên 1 máy)
# Mở file common/include/config.h và đổi:
#   AUTO_LOGIN_ENABLED = 0  (tắt - mỗi client tự login)
#   AUTO_LOGIN_ENABLED = 1  (bật - tự động login)
# Sau đó rebuild: make clean && make all

# Docker Compose commands
docker compose build server          # Build image with OpenSSL
docker compose up server              # Run server
docker compose down                   # Stop server
docker compose logs -f server         # View logs

# Database inspection
sqlite3 data/battleship.db ".schema"
sqlite3 data/battleship.db "SELECT * FROM users;"
sqlite3 data/battleship.db "SELECT * FROM sessions;"

# Clean database and sessions
rm -f data/battleship.db data/battleship.db-shm data/battleship.db-wal
rm -f ~/.battleship/session.txt
```

---

## 📈 Progress Tracking

**Phase 1 (Networking)**: ████████████████████ 100% COMPLETE ✅
**Phase 2 (Authentication)**: ████████████████████ 100% COMPLETE ✅
  - 2.1 Database Setup: ████████████████████ 100% ✅
  - 2.2 Password Hashing: ████████████████████ 100% ✅
  - 2.3 Auto-Login: ████████████████████ 100% ✅
  - 2.4 Database Tests: ████████████████████ 100% ✅
  - 2.5 Session Validation: ████████████████████ 100% ✅

**Phase 3 (Matchmaking)**: ███████████████░░░░░ 75% NEARLY COMPLETE ✅
  - 3.1 Player List Server: ████████████████████ 100% ✅
  - 3.2 Challenge System: ████████████████████ 100% ✅
  - 3.3 Lobby UI Integration: ████████████████████ 100% ✅ ⭐ NEW
  - 3.4 Auto-Matchmaking: ░░░░░░░░░░░░░░░░░░░░ 0% (Bonus - Optional)

**Phase 4 (Core Gameplay)**: ░░░░░░░░░░░░░░░░░░░░ 0% (Next Phase)
  - 4.1 Ship Placement Sync: ░░░░░░░░░░░░░░░░░░░░ 0%
  - 4.2 Turn-based Gameplay: ░░░░░░░░░░░░░░░░░░░░ 0%
  - 4.3 Match End & Results: ░░░░░░░░░░░░░░░░░░░░ 0%

**Phase 5 (Persistence & ELO)**: ░░░░░░░░░░░░░░░░░░░░ 0% (Not started)
**Phase 6 (Additional Features)**: ░░░░░░░░░░░░░░░░░░░░ 0% (Not started)

**Overall Progress**: ████████████░░░░░░░░ 60%

---

## 🎯 Points Expected

**Phase 2 Rubric** (6 points base + bonus):
- ✅ Đăng ký và quản lý tài khoản: **2 điểm** (Register with hashed passwords)
- ✅ Đăng nhập và quản lý phiên: **2 điểm** (Login with session management + server validation)
- ✅ Lưu thông tin: **1 điểm** (SQLite with WAL mode)
- ✅ **BONUS: Password hashing (SHA-256)**: **+0.5-1 điểm**
- ✅ **BONUS: Auto-login with server validation**: **+0.5-1 điểm** ⭐ ENHANCED
- ✅ **BONUS: Comprehensive test coverage (97 tests)**: **+0.5 điểm**

**Phase 2 Estimate**: **6.5-8.5 điểm** (base + bonuses)

**Phase 3 Rubric** (5 points base + bonus):
- ✅ Cung cấp danh sách người chơi: **2 điểm**
  - ✅ Server: PLAYER_LIST_REQUEST/RESPONSE handlers
  - ✅ Client: Real-time player list with status updates
  - ✅ UI: GTK TreeView with online/offline detection
- ✅ Chuyển lời thách đấu: **2 điểm**
  - ✅ Server: CHALLENGE_SEND handler with routing
  - ✅ Client: sendChallenge() API
  - ✅ Challenge notification system
- ✅ Từ chối/Nhận thách đấu: **1 điểm**
  - ✅ Server: CHALLENGE_RESPONSE handler
  - ✅ 60s timeout with auto-decline
  - ✅ Match creation on acceptance
- 🎁 **BONUS: Auto-matchmaking theo ELO**: **+0.5-1 điểm** (Optional)
  - Queue management system
  - ELO-based matching algorithm
  - Better UX with "Find Match" button

**Phase 3 Estimate**: **5 điểm** (base) + **0.5-1 điểm** (if bonus implemented)

---

## 🎉 Key Achievements

### Security & Best Practices:
- ✅ SHA-256 password hashing with random salt
- ✅ Constant-time password comparison
- ✅ Secure session file permissions (0600)
- ✅ SQL injection prevention (prepared statements)
- ✅ Session expiration (24h)
- ✅ WAL mode for concurrent database access
- ✅ Server-side token validation on auto-login ⭐ NEW
- ✅ Centralized configuration (config.h) ⭐ NEW

### User Experience:
- ✅ Auto-login on app restart
- ✅ Session persistence across restarts
- ✅ Seamless authentication flow
- ✅ Secure session validation before granting access ⭐ NEW

### Code Quality:
- ✅ 97 tests covering all features
- ✅ Comprehensive error handling
- ✅ Thread-safe database operations
- ✅ Clean architecture with separation of concerns
- ✅ Fixed critical enum collision bug ⭐ NEW

### DevOps:
- ✅ Docker support with multi-stage builds
- ✅ Automated integration test runner
- ✅ Database persistence in Docker volumes
- ✅ Health checks for containerized deployment

---

## 🚀 Phase 3: Matchmaking & Lobby (NEXT - 1 WEEK)

**Timeline**: 1 week | **Target Points**: 5 điểm

**Goal**: Enable players to find each other, send challenges, and start matches

---

### ✅ Phase 3.1: Player List Server (COMPLETE - 2 days)

**Server-Side Features:**
- ✅ Track online players with status (available/in_game/busy)
- ✅ `PLAYER_LIST_REQUEST` handler
  - Return list of online players with their status and ELO rating
  - Filter out players who are already in a game
- ✅ `PLAYER_STATUS_UPDATE` broadcast
  - Notify all clients when a player logs in/out
  - Notify when player status changes (available → in_game)
  - Real-time updates to all connected clients

**Database:**
- ✅ Use existing `users` table for player info
- ✅ Track online status in-memory (Server's PlayerManager)
- ✅ Use `matches` table to determine if player is in a game

**Protocol Messages:**
```cpp
// Added to protocol.h
PLAYER_LIST_REQUEST = 10,      // Request list of online players
PLAYER_LIST_RESPONSE = 11,     // Response with player list
PLAYER_STATUS_UPDATE = 12,     // Broadcast when player status changes
```

**Deliverable 1**: ✅ Server can track online players and broadcast status updates

**See detailed completion notes at top of document (lines 15-92)**

---

### ✅ Phase 3.2: Challenge System (COMPLETE - Days 3-4)

**Server-Side Challenge Flow:**
- ✅ `CHALLENGE_SEND` handler
  - ✅ Validate sender is authenticated and available
  - ✅ Validate target player exists and is online
  - ✅ Check target is not busy (status = available)
  - ✅ Route challenge to target player
  - ✅ Store pending challenge with timeout (60s)

- ✅ `CHALLENGE_RECEIVED` notification
  - ✅ Forward challenge to target client
  - ✅ Include challenger info (username, display_name, elo_rating)

- ✅ `CHALLENGE_RESPONSE` handler
  - ✅ Accept: Create match in database, set both players to in_game
  - ✅ Decline: Notify challenger
  - ✅ Timeout: Auto-decline after 60s, notify challenger

- ✅ Match creation on acceptance
  - ✅ Insert into `matches` table (status: waiting_for_ships)
  - ✅ Return match_id to both players
  - ✅ Broadcast status update (both players now IN_GAME)

**Protocol Messages:**
```cpp
// Added to protocol.h
CHALLENGE_SEND = 20,           // Send challenge to another player
CHALLENGE_RECEIVED = 21,       // Notify target of incoming challenge
CHALLENGE_RESPONSE = 22,       // Accept/decline challenge
MATCH_START = 23,              // Notify both players match is created
```

**Timeout Handling:**
- ✅ Track pending challenges with timestamp
- ✅ Background thread checks for expired challenges (60s)
- ✅ Auto-decline and notify both players

**Deliverable 2**: ✅ Server can route challenges, handle responses, and create matches

**See detailed completion notes at top of document (lines 15-152)**

---

### 📋 Phase 3.3: Lobby UI Integration (COMPLETE ✅)

**Client-Side Lobby UI:**
- ✅ Display player list (GTK TreeView)
  - ✅ Columns: Display Name, ELO Rating, Status
  - ✅ Real-time updates when players join/leave
  - ✅ Refresh on PLAYER_STATUS_UPDATE
  - ✅ Remove offline players from list

- ✅ Challenge button
  - ✅ Enabled when a player is selected and available
  - ✅ Sends CHALLENGE_SEND to server
  - ✅ Disabled during challenge pending

- ✅ Receive challenge dialog (GTK Dialog)
  - ✅ Show challenger info (name, ELO)
  - ✅ Accept/Decline buttons
  - ✅ Modal dialog with proper styling

- ✅ Match start transition
  - ✅ On MATCH_START, transition to ship placement screen
  - ✅ Store match_id for gameplay

- ✅ Disconnect detection
  - ✅ Server broadcasts STATUS_OFFLINE on disconnect
  - ✅ UI removes offline players from TreeView

**Client Network API:**
```cpp
// Added to client_network.h
void requestPlayerList(PlayerListCallback callback);
void sendChallenge(uint32_t target_user_id, uint32_t time_limit, bool random_placement, SendChallengeCallback callback);
void respondToChallenge(uint32_t challenge_id, bool accept);

// Event callbacks
void setPlayerStatusCallback(PlayerStatusCallback callback);
void setChallengeReceivedCallback(ChallengeReceivedCallback callback);
void setMatchStartCallback(MatchStartCallback callback);
```

**Implementation Files:**
- `client/src/ui_lobby.cpp` - Complete lobby screen with TreeView, callbacks
- `client/src/ui_manager.cpp` - Challenge dialog, screen management
- `client/src/client_network.cpp` - Network API, message handlers
- `tests/manual/test_lobby_flow.cpp` - Integration test ✅
- `tests/manual/test_disconnect.cpp` - Disconnect detection test ✅

**Bug Fixes:**
- ✅ Fixed enum collision (PendingRequest::PLAYER_LIST shadowing MessageType::PLAYER_LIST)
- ✅ Fixed disconnect detection (server now broadcasts offline status)
- ✅ Fixed UI port configuration (using SERVER_PORT from config.h)
- ✅ Fixed auto-login disable for demo purposes

**Deliverable 3**: ✅ Players can see each other, send/receive challenges, start matches, and detect disconnects!

---

### 📋 Phase 3.4: Auto-Matchmaking System (1-2 ngày)

**Server-Side Matchmaking Queue:**
- [ ] `QUEUE_JOIN` handler
  - Add player to matchmaking queue
  - Store player's ELO rating and timestamp
  - Set player status to "in_queue"
  - Broadcast PLAYER_STATUS_UPDATE

- [ ] `QUEUE_LEAVE` handler
  - Remove player from queue
  - Set player status back to "available"
  - Broadcast PLAYER_STATUS_UPDATE

- [ ] ELO-based matching algorithm
  - Background thread checks queue every 2-3 seconds
  - Match players with similar ELO (±100-200 range)
  - Prioritize longest waiting players
  - Create match automatically when pair found
  - Remove both players from queue
  - Send MATCH_CREATED to both players

- [ ] Queue timeout handling
  - Max wait time: 60 seconds
  - Gradually expand ELO range if no match (±100 → ±200 → ±300)
  - Notify player if no match found after timeout
  - Auto-remove from queue

**Matching Algorithm:**
```
Every 2-3 seconds:
  For each player in queue (sorted by wait time DESC):
    Find opponent where:
      - abs(player.elo - opponent.elo) <= ELO_RANGE
      - opponent is not the player
      - opponent hasn't been checked yet
    If found:
      Create match
      Remove both from queue
      Notify both players
```

**ELO Range Expansion:**
- 0-20s wait: ±100 ELO
- 20-40s wait: ±200 ELO
- 40-60s wait: ±300 ELO
- 60s+: Timeout, notify "No match found"

**Protocol Messages:**
```cpp
// Add to protocol.h
QUEUE_JOIN = 30,               // Join matchmaking queue
QUEUE_LEAVE = 31,              // Leave matchmaking queue
QUEUE_STATUS = 32,             // Queue status update (position, wait time)
MATCH_FOUND = 33,              // Notify players match was found
QUEUE_TIMEOUT = 34,            // No match found within timeout
```

**Client-Side Queue UI:**
- [ ] "Find Match" button in lobby
  - Joins queue on click
  - Shows "Searching for opponent..." status
  - Displays wait time counter
  - Shows estimated ELO range

- [ ] Cancel button while in queue
  - Leaves queue
  - Returns to lobby

- [ ] Match found notification
  - Show opponent info (name, ELO)
  - Auto-transition to ship placement
  - 5s countdown before match starts

**Client Network API:**
```cpp
// Add to client_network.h
void joinQueue(QueueCallback callback);
void leaveQueue(QueueCallback callback);
```

**Deliverable 4**: Players can auto-match based on ELO rating

---

### 📦 Phase 3 Deliverables

**Core Functionality (5 điểm):**
1. ✅ **COMPLETE** - Cung cấp danh sách người chơi: **2 điểm**
   - ✅ List online players with status
   - ✅ Real-time updates

2. ✅ **COMPLETE** - Chuyển lời thách đấu: **2 điểm**
   - ✅ Send challenge to specific player
   - ✅ Route challenge through server
   - ✅ Notify target player

3. ✅ **COMPLETE** - Từ chối/Nhận thách đấu: **1 điểm**
   - ✅ Accept/decline responses
   - ✅ Timeout handling (60 seconds)
   - ✅ Create match on acceptance

**Bonus Features:**
4. 🎁 **Auto-matchmaking theo ELO**: **+0.5-1 điểm**
   - Queue management system
   - ELO-based matching algorithm
   - Automatic pairing
   - Better UX with "Find Match" button

**Total Phase 3 Points**: **5-6 điểm** (base + bonus)

---

### 🔧 Phase 3 Implementation Plan

**Week+ Breakdown (8-9 ngày):**

- ✅ **Days 1-2**: Player list server (3.1) - COMPLETE
  - ✅ Implement PLAYER_LIST_REQUEST/RESPONSE
  - ✅ Implement PLAYER_STATUS_UPDATE broadcast
  - ✅ Test with multiple clients
  - ✅ **Deliverable**: Server tracks and broadcasts player status

- ✅ **Days 3-4**: Challenge system server (3.2) - COMPLETE
  - ✅ Implement CHALLENGE_SEND/RECEIVED/RESPONSE
  - ✅ Implement timeout handling (60s)
  - ✅ Implement match creation on acceptance
  - ✅ Test challenge flow (send, accept, decline, timeout)
  - ✅ **Deliverable**: Players can challenge each other

- 🚀 **Days 5-6**: Lobby UI (3.3) - NEXT
  - Build player list view (GTK TreeView)
  - Build challenge dialog with countdown
  - Integrate with server
  - Test end-to-end flow (send challenge → accept → match created)
  - **Deliverable**: Full challenge UI working

- ✅ **Days 5-7**: Lobby UI Integration (3.3) - COMPLETE
  - ✅ Built player list view (GTK TreeView)
  - ✅ Built challenge dialog with Accept/Decline
  - ✅ Integrated with server (real-time updates)
  - ✅ Fixed disconnect detection bug
  - ✅ Fixed enum collision bug
  - ✅ **Deliverable**: Full challenge UI working + tested

- 📋 **Days 8-9** (Optional): Auto-matchmaking (3.4) - BONUS
  - Implement queue management (join/leave)
  - Implement ELO-based matching algorithm
  - Implement background matching thread
  - Build "Find Match" UI with queue status
  - Test matchmaking with multiple clients
  - **Deliverable**: Auto-matchmaking working (+0.5-1 điểm)

---

## 🎮 Phase 4: Core Gameplay Multiplayer (1.5 tuần) - 6 điểm

**Status**: Not Started
**Duration**: 10-12 ngày
**Points**: 6 điểm (Core gameplay requirements)

---

### 📋 Phase 4.1: Ship Placement Sync (2-3 ngày)

**Server-Side Ship Placement:**
- [ ] `SHIP_PLACEMENT` message handler
  - Receive ship positions from client
  - Validate ship placement rules:
    - All 5 ships placed (Carrier=5, Battleship=4, Cruiser=3, Submarine=3, Destroyer=2)
    - No overlapping ships
    - Ships within 10x10 grid
    - Ships don't touch each other
  - Store in `match_boards` table (board_data as JSON)
  - Mark player as "ready"

- [ ] Wait for both players ready
  - Track ready status for both players
  - When both ready, start match
  - Send `MATCH_READY` to both players
  - Set first turn (random or based on who sent challenge)

- [ ] Board validation
  - Server validates ship placement before accepting
  - Return error if invalid placement
  - Client can retry placement

**Client-Side Ship Placement UI:**
- [ ] Ship placement screen (GTK)
  - 10x10 grid for player's board
  - Drag-and-drop ships from sidebar
  - Rotate ship button (Horizontal ↔ Vertical)
  - Visual feedback for valid/invalid placement
  - Show placement rules

- [ ] Ship placement logic
  - Track all 5 ships placement
  - Validate placement rules locally (instant feedback)
  - Enable "Ready" button when all ships placed
  - Send SHIP_PLACEMENT to server on "Ready"

- [ ] Waiting screen
  - "Waiting for opponent to place ships..."
  - Show countdown timer
  - Cancel button (forfeit match)

**Protocol Messages:**
```cpp
// Add to protocol.h
SHIP_PLACEMENT = 30,           // Client sends ship positions
PLACEMENT_RESULT = 31,         // Server validates placement
MATCH_READY = 32,              // Both players ready, match starts
```

**Database:**
```sql
-- match_boards table already exists
-- Store board_data as JSON:
{
  "ships": [
    {"type": "CARRIER", "row": 0, "col": 0, "orientation": "HORIZONTAL"},
    ...
  ]
}
```

**Deliverable 1**: ✅ Both players can place ships, server validates, match starts when both ready

---

### 📋 Phase 4.2: Turn-based Gameplay (3-4 ngày)

**Server-Side Move Handling:**
- [ ] `MOVE` handler
  - Validate it's player's turn
  - Validate coordinate is valid (0-9, not already shot)
  - Load opponent's board from database
  - Check if shot hits a ship
  - Determine result: MISS, HIT, or SUNK
  - Update board state in database
  - Save move to `match_moves` table
  - Check for win condition (all ships sunk)

- [ ] `MOVE_RESULT` response
  - Send result to shooter: {result, ship_type_if_sunk}
  - Send `TURN_UPDATE` to both players
  - If win, send `MATCH_END`
  - Switch turn to other player

- [ ] Turn timer (60s per turn)
  - Background thread tracks turn start time
  - If timeout, auto-forfeit current player
  - Send `TURN_TIMEOUT` to both players

- [ ] Game state synchronization
  - Both players always have consistent game state
  - Server is source of truth
  - Clients only display state, don't compute

**Client-Side Gameplay UI:**
- [ ] Dual board view
  - Left: Player's board (show own ships + opponent's shots)
  - Right: Opponent's board (show own shots + hit/miss markers)
  - Clear visual distinction between boards

- [ ] Turn indicator
  - "Your turn" / "Opponent's turn"
  - Turn timer countdown (60s)
  - Disable clicks when not your turn

- [ ] Shot input
  - Click opponent's board to shoot
  - Confirmation dialog? (optional)
  - Send MOVE to server
  - Wait for MOVE_RESULT

- [ ] Shot result animations
  - Miss: Blue splash animation
  - Hit: Red explosion animation
  - Sunk: Show entire ship on opponent's board
  - Sound effects (optional)

- [ ] Game info panel
  - Ships remaining (yours and opponent's)
  - Move history (last 5 moves)
  - Match time elapsed

**Protocol Messages:**
```cpp
MOVE = 33,                     // Player shoots at coordinate
MOVE_RESULT = 34,              // Result: miss/hit/sunk
TURN_UPDATE = 35,              // Whose turn it is
TURN_TIMEOUT = 36,             // Turn timer expired
```

**Deliverable 2**: ✅ Players can take turns shooting, see hit/miss results, game state synced

---

### 📋 Phase 4.3: Match End & Results (2-3 ngày)

**Server-Side Match End:**
- [ ] Win condition detection
  - Player wins when all opponent ships are sunk
  - Draw condition: mutual timeout/disconnect
  - Forfeit: player disconnects or times out repeatedly

- [ ] `MATCH_END` message
  - Send to both players with result
  - Include: winner_id, reason (win/draw/forfeit), final_stats

- [ ] Save match results
  - Update `matches` table:
    - winner_id
    - end_time
    - total_moves
    - status = 'completed'
  - Prepare for ELO update (Phase 5)

- [ ] Cleanup
  - Remove match from active matches
  - Set both players status back to AVAILABLE
  - Broadcast PLAYER_STATUS_UPDATE

**Client-Side Results Screen:**
- [ ] Match end dialog
  - Show result: "Victory!" / "Defeat" / "Draw"
  - Display match statistics:
    - Total moves
    - Hit accuracy (hits / total shots)
    - Time elapsed
    - Ships sunk (yours and opponent's)

- [ ] Action buttons
  - "Rematch" (send REMATCH_REQUEST to opponent)
  - "Return to Lobby"
  - "View Replay" (Phase 5)

- [ ] Results transition
  - Smooth transition from gameplay to results
  - Keep boards visible in results screen
  - Celebration animation for winner

**Protocol Messages:**
```cpp
MATCH_END = 37,                // Match finished
MATCH_STATS = 38,              // Detailed match statistics
```

**Deliverable 3**: ✅ Match ends properly, results displayed, players return to lobby

**Phase 4 Total Points**: **6 điểm**
- Chuyển thông tin nước đi: **2 điểm** (MOVE handler + synchronization)
- Kiểm tra tính hợp lệ: **1 điểm** (Move validation + ship placement)
- Xác định kết quả ván cờ: **1 điểm** (Win detection + MATCH_END)
- Ship placement sync: **2 điểm** (implied from multiplayer requirement)

---

## 💾 Phase 5: Persistence & ELO (1 tuần) - 6 điểm

**Status**: Not Started
**Duration**: 7-9 ngày
**Points**: 6 điểm (Logging + ELO + Replay)

---

### 📋 Phase 5.1: Move Logging (1-2 ngày)

**Server-Side Move Logging:**
- [ ] Save every move to database
  - Table: `match_moves` (match_id, player_id, move_number, row, col, result, timestamp)
  - Save in real-time during gameplay
  - Include complete move data for replay

- [ ] Move sequence tracking
  - Maintain move order (move_number)
  - Track whose move it was
  - Record exact timestamp

- [ ] Match data completeness
  - After match ends, verify all moves logged
  - Store final board states
  - Link to match result

**Deliverable 1**: ✅ All moves logged to database with complete data

**Points**: **1 điểm** (Ghi log)

---

### 📋 Phase 5.2: ELO Rating System (2-3 ngày)

**Server-Side ELO Calculation:**
- [ ] ELO formula implementation
  - K-factor: 32 (standard for chess)
  - Formula: `new_elo = old_elo + K * (actual_score - expected_score)`
  - Expected score: `1 / (1 + 10^((opponent_elo - player_elo)/400))`

- [ ] Update ratings after match
  - Calculate for both players (winner gains, loser loses)
  - Update `users` table (elo_rating column)
  - Track highest ELO reached (new column: `highest_elo`)
  - Update win/loss/draw counts

- [ ] `ELO_UPDATE` message
  - Send to both players after match
  - Include: old_elo, new_elo, change (+/-), new_rank

- [ ] ELO history tracking
  - New table: `elo_history` (user_id, match_id, old_elo, new_elo, timestamp)
  - Enables ELO graph over time

**Client-Side ELO Display:**
- [ ] Show ELO change in results screen
  - "+25 ELO" (green) or "-18 ELO" (red)
  - Progress bar to next rank
  - Rank name (Bronze/Silver/Gold/Platinum/Diamond)

- [ ] Profile screen
  - Current ELO + rank
  - Highest ELO reached
  - ELO graph over last 20 matches
  - Win/loss/draw record

**Deliverable 2**: ✅ ELO ratings update after matches, displayed to players

**Points**: **2 điểm** (Xây dựng hệ thống tính điểm)

---

### 📋 Phase 5.3: Match Replay System (2-3 ngày)

**Server-Side Replay:**
- [ ] `REPLAY_REQUEST` handler
  - Load match data from `matches` table
  - Load all moves from `match_moves` table (ordered by move_number)
  - Load both players' ship placements from `match_boards`
  - Package into REPLAY_DATA response

- [ ] Replay data format
  - Match metadata (players, result, duration)
  - Ship positions for both players
  - Move sequence with results
  - Timestamps for each move

**Client-Side Replay UI:**
- [ ] Replay controls
  - Play/Pause button
  - Next move / Previous move
  - Speed control (1x, 2x, 4x)
  - Jump to move number

- [ ] Replay visualization
  - Show boards for both players
  - Animate moves in sequence
  - Show shot results (hit/miss/sunk)
  - Display move timer

- [ ] Replay info panel
  - Current move number / total moves
  - Whose turn
  - Move history sidebar

**Protocol Messages:**
```cpp
REPLAY_REQUEST = 50,           // Request replay for match_id
REPLAY_DATA = 51,              // Send complete replay data
```

**Deliverable 3**: ✅ Players can watch replays of completed matches

**Points**: **3 điểm** (Chuyển kết quả và log **2 điểm** + Lưu thông tin replay **1 điểm**)

---

## ✨ Phase 6: Additional Features & Polish (1 tuần) - 2 điểm + polish

**Status**: Not Started
**Duration**: 5-7 ngày
**Points**: 2 điểm (Pause/Draw/Rematch)

---

### 📋 Phase 6.1: Match Actions (2-3 ngày)

**Pause/Resume:**
- [ ] `PAUSE_REQUEST` handler
  - Player can request pause during their turn
  - Send to opponent for approval
  - If approved, pause turn timer
  - Max pause: 5 minutes

- [ ] `PAUSE_RESPONSE` handler
  - Opponent accepts/declines pause
  - If accepted, both clients show "Paused" state
  - Resume button visible to both

**Draw Offer:**
- [ ] `DRAW_OFFER` handler
  - Player can offer draw at any time
  - Send to opponent for approval
  - Dialog appears for opponent

- [ ] `DRAW_RESPONSE` handler
  - Accept: Match ends as draw, no ELO change
  - Decline: Continue playing

**Resign:**
- [ ] `RESIGN` handler
  - Player forfeits match immediately
  - Opponent wins automatically
  - Update ELO accordingly
  - Return both to lobby

**Rematch:**
- [ ] `REMATCH_REQUEST` handler
  - After match ends, player can request rematch
  - Send to opponent
  - 30s timeout for response

- [ ] `REMATCH_RESPONSE` handler
  - Accept: Create new match with same players
  - Decline: Return to lobby
  - New match starts at ship placement

**Protocol Messages:**
```cpp
PAUSE_REQUEST = 40,
PAUSE_RESPONSE = 41,
DRAW_OFFER = 42,
DRAW_RESPONSE = 43,
RESIGN = 44,
REMATCH_REQUEST = 45,
REMATCH_RESPONSE = 46,
```

**Deliverable 1**: ✅ Players can pause, offer draw, resign, and request rematch

**Points**: **2 điểm** (Xin ngừng/mời hòa **1 điểm** + Xin đấu lại **1 điểm**)

---

### 📋 Phase 6.2: Polish & Stability (2-3 ngày)

**Error Handling:**
- [ ] Graceful disconnection handling
  - Detect client disconnect during match
  - Auto-resign after 60s
  - Notify opponent
  - Update match result

- [ ] Network error recovery
  - Retry failed messages
  - Reconnect on temporary disconnect
  - Restore match state after reconnect

- [ ] Input validation everywhere
  - Server validates all client inputs
  - Prevent cheating/exploits
  - Rate limiting on requests

**Performance & Memory:**
- [ ] Memory leak checking (Valgrind)
  - Fix any leaks in network code
  - Fix any leaks in UI code
  - Clean shutdown handling

- [ ] Load testing
  - 50+ concurrent players
  - 20+ simultaneous matches
  - Stress test matchmaking queue

**UI/UX Polish:**
- [ ] Animations and transitions
  - Smooth screen transitions
  - Shot result animations
  - Loading indicators

- [ ] Sound effects (optional)
  - Shot sounds (miss vs hit)
  - Ship sunk sound
  - Victory/defeat sounds

- [ ] Tooltips and help
  - Hover tooltips on buttons
  - Help dialog explaining game rules
  - Keyboard shortcuts

**Deliverable 2**: ✅ Stable, polished application ready for demo

---

## 📝 Technical Notes

### Password Hashing Details:
- **Algorithm**: SHA-256 (OpenSSL implementation)
- **Salt**: 16 random bytes (32 hex chars)
- **Format**: `salt$hash` (salt + dollar + hash)
- **Hash Length**: 64 hex chars (32 bytes)
- **Comparison**: Constant-time to prevent timing attacks

### Session Management:
- **Server-side**: Database with 24h expiration
- **Client-side**: File-based persistence (~/.battleship/session.txt)
- **Token Format**: `token_{user_id}_{random_hash}`
- **Validation**: Server checks token + expiration on each request

### Database Schema:
- **WAL Mode**: Enabled for concurrent read/write
- **Indexes**: On username (unique), session_token (unique)
- **Foreign Keys**: Enabled for referential integrity
- **Prepared Statements**: All queries use prepared statements

### Testing Strategy:
- **Unit Tests**: Test individual components in isolation
- **Integration Tests**: Test client-server interaction with real server
- **Test Database**: Temporary database per test (`/tmp/test_battleship_*.db`)
- **Cleanup**: Automatic cleanup of test artifacts

---

## 🐛 Known Issues & Solutions

**Issue**: Integration tests fail if server not running
**Solution**: Use `./run_integration_tests.sh` which auto-starts server

**Issue**: Database locked errors
**Solution**: WAL mode enabled - allows concurrent access

**Issue**: Session file permissions
**Solution**: Automatically set to 0600 on creation

**Issue**: Docker build missing OpenSSL
**Solution**: ✅ FIXED - Added libssl-dev and libssl3 to Dockerfile

**Issue**: Enum name collision (VALIDATE_SESSION)
**Solution**: ✅ FIXED - Use explicit namespace `MessageType::VALIDATE_SESSION`

---

## 📊 Phase 3 Technical Architecture (Preview)

**Server Components:**
```
Server
  ├── PlayerManager (NEW)
  │    ├── Track online players
  │    ├── Broadcast status updates
  │    └── Get available players list
  │
  ├── ChallengeManager (NEW)
  │    ├── Route challenges between players
  │    ├── Handle accept/decline responses
  │    ├── Track pending challenges with timeout
  │    └── Create matches on acceptance
  │
  ├── MatchmakingQueue (NEW - BONUS)
  │    ├── Queue management (join/leave)
  │    ├── ELO-based matching algorithm
  │    ├── Background matching thread
  │    └── Timeout handling
  │
  └── MatchManager (EXISTING)
       └── Create/manage match records in DB
```

**Protocol Flow Examples:**

**Direct Challenge Flow:**
```
Client A                    Server                    Client B
   |                          |                          |
   |--- CHALLENGE_SEND -----> |                          |
   |                          |--- CHALLENGE_RECEIVED -->|
   |                          |                          |
   |                          | <-- CHALLENGE_RESPONSE --|
   | <-- MATCH_CREATED ------ |                          |
   |                          |------ MATCH_CREATED ---->|
```

**Auto-Matchmaking Flow:**
```
Client A                    Server                    Client B
   |                          |                          |
   |--- QUEUE_JOIN ---------> |                          |
   |                          | <-------- QUEUE_JOIN ----|
   |                          |                          |
   |      (matching thread runs, finds pair)             |
   |                          |                          |
   | <-- MATCH_FOUND -------- |                          |
   |                          |-------- MATCH_FOUND ---->|
   | <-- MATCH_CREATED ------ |                          |
   |                          |------ MATCH_CREATED ---->|
```

---

Last Updated: 2025-12-04
**Phase 2: Authentication System - 100% COMPLETE! 🎉**
**Phase 3.1: Player List Server - 100% COMPLETE! 🎉**
**Phase 3.2: Challenge System - 100% COMPLETE! 🎉**
**Phase 3.3: Lobby UI - NEXT STEP! 🚀**
