# Battleship Multiplayer - Development Notes

## Current Phase: 2.1 - Database Setup

### Completed ✅

**Phase 1.1-1.3: Networking Foundation**
- ✅ Basic Socket Server (multi-client TCP server)
- ✅ Protocol Implementation (binary message format, auth messages)
- ✅ Client Networking Implementation (GTK UI integration)
- ✅ All tests passing (68 unit tests + 21 integration tests)

**Phase 2.1: Database Setup (IN PROGRESS)**
- ✅ Created DatabaseManager class with SQLite integration
- ✅ Defined database schema:
  - `users` table (user_id, username, password_hash, display_name, elo_rating, created_at, last_login)
  - `sessions` table (session_id, user_id, session_token, created_at, expires_at)
  - `matches` table (match_id, player1_id, player2_id, winner_id, status, created_at, ended_at)
  - `match_boards` table (board_id, match_id, user_id, ship_data)
  - `match_moves` table (move_id, match_id, player_id, move_number, x, y, result, timestamp)
- ✅ Implemented CRUD operations for users table
- ✅ Implemented CRUD operations for sessions table
- ✅ Migrated AuthHandler from in-memory to database
- ✅ Added session expiration logic (24h default)
- ✅ Server builds successfully with database
- ✅ Database file created: `data/battleship.db`
- ✅ Docker Compose updated with:
  - `data` volume mount for database persistence
  - `sqlite3` CLI tool in runtime image for debugging
  - Database persists across container restarts

### ✅ PHASE 2.1 - COMPLETE!

**All Integration Tests Passing** 🎉
- ✅ Client-Server tests: 9/9 passing
- ✅ Authentication tests: 7/7 passing
- ✅ E2E Client Auth tests: 5/5 passing
- **Total: 21/21 integration tests PASSED!**

**Root Cause Fixed:**
- Issue was "readonly database" error
- SQLite couldn't write due to default journaling mode
- **Solution**: Enabled WAL (Write-Ahead Logging) mode
- WAL allows concurrent readers + 1 writer
- Added database cleanup in test script

### TODOs - Phase 2 Remaining

1. **Implement Client Auto-Login** 🟡 MEDIUM PRIORITY
   - [ ] Store session token in client (UIManager)
   - [ ] On startup, check if token exists
   - [ ] Send token validation request to server
   - [ ] Auto-login if token valid
   - [ ] Clear token if expired
   - Location: `client/src/ui_manager.cpp`, `client/include/ui_manager.h`

4. **Write Unit Tests for DatabaseManager** 🟡 MEDIUM PRIORITY
   - [ ] Test user CRUD operations
   - [ ] Test session CRUD operations
   - [ ] Test session expiration
   - [ ] Test match operations
   - [ ] Test transaction safety
   - Create: `tests/unit_tests/server/test_database.cpp`

5. **Session Cleanup Background Task** 🟢 LOW PRIORITY
   - [ ] Add periodic cleanup of expired sessions
   - [ ] Run `db_->cleanupExpiredSessions()` every hour
   - [ ] Use timer thread in Server class
   - Location: `server/src/server.cpp`

6. **Update Documentation** 🟢 LOW PRIORITY
   - [ ] Document database schema in detail
   - [ ] Add ER diagram
   - [ ] Document session management flow
   - Create: `docs/database_schema.md`

### Next Phase: 2.2 - Authentication Server-side (Full Implementation)

After completing 2.1, these are the next tasks:

1. **Password Security**
   - Implement bcrypt or SHA-256 hashing
   - Salt generation and storage
   - Secure password validation

2. **Session Management Improvements**
   - Token refresh mechanism
   - Multi-device session support
   - Session revocation

3. **Error Handling**
   - Comprehensive error messages
   - Input validation
   - SQL injection prevention (already handled by prepared statements)

### Testing Status

**Unit Tests**: 68/68 passing ✅
- Board: 17 tests
- Match: 16 tests
- Auth Messages: 13 tests
- Network: 11 tests
- ClientNetwork: 11 tests

**Integration Tests**: 21/21 passing ✅
- Client-Server: 9/9 passing ✅
- Authentication: 7/7 passing ✅
- E2E Client Auth: 5/5 passing ✅

### Files Modified in Phase 2.1

**New Files:**
- `server/include/database.h` - DatabaseManager class header (240 lines)
- `server/src/database.cpp` - DatabaseManager implementation (680 lines)
- `data/battleship.db` - SQLite database file (auto-created, WAL mode)

**Modified Files:**
- `server/include/auth_handler.h` - Updated to use DatabaseManager
- `server/src/auth_handler.cpp` - Migrated to database operations (210 lines)
- `server/include/server.h` - Added DatabaseManager member
- `server/src/server.cpp` - Initialize and use database, added WAL mode
- `run_integration_tests.sh` - Added database cleanup before tests
- `Dockerfile` - Added sqlite3 CLI tool for debugging
- `docker-compose.yml` - Already had data volume mount

### Architecture Notes

**Database Layer:**
```
Server
  └── DatabaseManager (owns SQLite connection)
       ├── User CRUD
       ├── Session CRUD
       ├── Match CRUD (for Phase 4-5)
       └── Move logging (for Phase 5)

AuthHandler
  └── Uses DatabaseManager (pointer, not owned)
       ├── Register: db_->createUser()
       ├── Login: db_->getUserByUsername() + db_->createSession()
       └── Logout: db_->deleteSession()
```

**Session Expiration:**
- Default: 24 hours
- Stored in `sessions.expires_at` (Unix timestamp)
- Validated on each request via `db_->validateSession()`
- Auto-deleted when expired

**Thread Safety:**
- SQLite handles concurrent access internally
- DatabaseManager methods are thread-safe
- Prepared statements created per-call

### Quick Commands

```bash
# Build server with database
make clean && make server

# Run integration tests
./run_integration_tests.sh

# Run in Docker
./run_integration_tests.sh --docker

# Docker Compose commands
docker compose build server          # Build image
docker compose up server              # Run server
docker compose down                   # Stop server
docker compose logs -f server         # View logs

# Check database (sqlite3 installed)
sqlite3 data/battleship.db ".schema"
sqlite3 data/battleship.db "SELECT * FROM users;"
sqlite3 data/battleship.db "SELECT * FROM sessions;"
sqlite3 data/battleship.db "SELECT COUNT(*) FROM users;"

# Clean database
rm data/battleship.db

# Access sqlite3 inside running container
docker compose exec server sqlite3 /app/data/battleship.db
```

### Progress Tracking

**Phase 1**: ████████████████████ 100% COMPLETE ✅
**Phase 2.1**: ████████████████████ 100% COMPLETE ✅
**Phase 2.2**: ░░░░░░░░░░░░░░░░░░░░ 0% (Not started)
**Phase 2.3**: ░░░░░░░░░░░░░░░░░░░░ 0% (Not started)

**Total Phase 2**: ████████████░░░░░░░░ 60%

### Points Expected

**Phase 2 Rubric** (6 points total):
- Đăng ký và quản lý tài khoản: 2 điểm ✅ (COMPLETE)
- Đăng nhập và quản lý phiên: 2 điểm ✅ (COMPLETE)
- Lưu thông tin: 1 điểm ✅ (COMPLETE - SQLite with WAL mode)
- Password hashing: (using plaintext for now, can add bcrypt later for bonus)

**Current Estimate**: **5-6 points CONFIRMED** ✅

**Key Achievement:**
- Complete authentication system with persistent SQLite database
- All 21 integration tests passing
- Docker support with volume persistence
- Session management with 24h expiration
- Thread-safe concurrent access with WAL mode

---

Last Updated: 2025-11-17 00:52 UTC
**Phase 2.1: Database Setup - COMPLETE! 🎉**
