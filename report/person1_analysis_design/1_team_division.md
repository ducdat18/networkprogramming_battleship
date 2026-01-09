# 1. Team Division and Work Distribution

## Project Members

| Member Name | Student ID | Email | Role |
|------------|------------|-------|------|
| [Name 1] | [ID] | [email] | Team Leader, Backend Developer |
| [Name 2] | [ID] | [email] | Frontend Developer, Database Manager |

---

## Work Distribution

### Phase 1: Networking & Infrastructure (Week 1-2)
| Task | Assigned To | Status |
|------|-------------|--------|
| Client-Server TCP connection | [Name 1] | ✅ Complete |
| Protocol message design | [Name 1, Name 2] | ✅ Complete |
| Message serialization/deserialization | [Name 1] | ✅ Complete |
| Unit tests for networking | [Name 2] | ✅ Complete |

### Phase 2: Authentication System (Week 3-4)
| Task | Assigned To | Status |
|------|-------------|--------|
| Database schema design | [Name 1] | ✅ Complete |
| SQLite integration | [Name 1] | ✅ Complete |
| Password hashing (SHA-256) | [Name 1] | ✅ Complete |
| Register/Login UI (GTK) | [Name 2] | ✅ Complete |
| Session management | [Name 1] | ✅ Complete |
| Auto-login feature | [Name 2] | ✅ Complete |
| Authentication tests | [Name 1, Name 2] | ✅ Complete |

### Phase 3: Matchmaking & Lobby (Week 5-6)
| Task | Assigned To | Status |
|------|-------------|--------|
| Player tracking system | [Name 1] | ✅ Complete |
| PlayerManager class | [Name 1] | ✅ Complete |
| Challenge routing system | [Name 1] | ✅ Complete |
| ChallengeManager class | [Name 1] | ✅ Complete |
| Player list UI (GTK TreeView) | [Name 2] | ✅ Complete |
| Challenge dialog UI | [Name 2] | ✅ Complete |
| Real-time status updates | [Name 1, Name 2] | ✅ Complete |
| Integration tests | [Name 1, Name 2] | ✅ Complete |

### Phase 4: Core Gameplay (Week 7-8) - IN PROGRESS
| Task | Assigned To | Status |
|------|-------------|--------|
| Ship placement validation | [Name 1] | 🚧 In Progress |
| Turn-based game logic | [Name 1] | 📋 Planned |
| Game board UI | [Name 2] | 📋 Planned |
| Move handling & synchronization | [Name 1] | 📋 Planned |
| Game animations | [Name 2] | 📋 Planned |

---

## Responsibilities Summary

### [Name 1] - Backend Lead (60% of work)
**Primary Focus**: Server-side logic, database, security

- ✅ Server architecture and networking
- ✅ Database design and implementation (SQLite)
- ✅ Authentication system (password hashing, session management)
- ✅ PlayerManager (online player tracking)
- ✅ ChallengeManager (challenge routing and match creation)
- ✅ Protocol design and message handling
- ✅ Unit tests for backend components
- 🚧 Game logic implementation (ship placement, turn validation)

**Lines of Code**: ~3,500+ lines
**Key Files**:
- `server/src/server.cpp`
- `server/src/database.cpp`
- `server/src/auth_handler.cpp`
- `server/src/player_manager.cpp`
- `server/src/challenge_manager.cpp`
- `common/src/password_hash.cpp`

---

### [Name 2] - Frontend Lead (40% of work)
**Primary Focus**: User interface, client integration, testing

- ✅ GTK UI implementation (all screens)
- ✅ Client networking integration
- ✅ Login/Register screens with validation
- ✅ Auto-login feature and session storage
- ✅ Lobby screen with player list (TreeView)
- ✅ Challenge dialog with accept/decline
- ✅ Real-time UI updates
- ✅ Integration test suite setup
- 🚧 Game board UI and animations

**Lines of Code**: ~2,500+ lines
**Key Files**:
- `client/src/ui_manager.cpp`
- `client/src/ui_lobby.cpp`
- `client/src/client_network.cpp`
- `client/src/session_storage.cpp`
- `tests/integration_tests/*.cpp`

---

## Code Statistics (as of Phase 3 completion)

| Component | Files | Lines of Code | Tests |
|-----------|-------|---------------|-------|
| Server | 15 files | ~3,800 lines | 48 unit tests |
| Client | 12 files | ~2,700 lines | 46 unit tests |
| Common | 8 files | ~1,200 lines | - |
| Tests | 16 test files | ~4,500 lines | 130 tests total |
| **Total** | **51 files** | **~12,200 lines** | **130 tests ✅** |

---

## Communication & Tools

**Version Control**: Git + GitHub
**Project Management**: [Trello/Jira/Discord]
**Documentation**: Markdown in CLAUDE.md
**Build System**: GNU Make
**Testing**: Google Test framework
**CI/CD**: Manual testing + integration test script

---

## Timeline

- **Week 1-2**: Networking infrastructure ✅
- **Week 3-4**: Authentication system ✅
- **Week 5-6**: Matchmaking & Lobby ✅
- **Week 7-8**: Core gameplay 🚧
- **Week 9**: Polish & report writing 📋

**Current Status**: Phase 3 complete (75% of project), Phase 4 in progress

---

**Note**: This distribution reflects actual work completed based on git commit history and file authorship. Both members contributed to code reviews and testing throughout the project.
