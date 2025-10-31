# 📁 CẤU TRÚC DỰ ÁN BATTLESHIP ONLINE

## 🎯 Tổng quan

Dự án được tổ chức theo kiến trúc Client-Server với code được chia thành các module rõ ràng.

```
battleship/
│
├── 📂 client/              # CLIENT APPLICATION
│   ├── include/
│   │   └── ui_manager.h    # Quản lý giao diện GTK
│   └── src/
│       ├── main.cpp        # Entry point client
│       └── ui_manager.cpp  # Implementation UI với GTK+Cairo
│
├── 📂 server/              # SERVER APPLICATION
│   ├── include/            # Server headers (TODO)
│   │   ├── server.h
│   │   ├── auth_manager.h
│   │   ├── match_manager.h
│   │   └── database.h
│   └── src/                # Server sources (TODO)
│       ├── main.cpp
│       ├── server.cpp
│       ├── auth_manager.cpp
│       ├── match_manager.cpp
│       └── database.cpp
│
├── 📂 common/              # SHARED CODE
│   ├── include/
│   │   ├── protocol.h      # Network protocol definitions
│   │   └── game_state.h    # Game logic & state management
│   └── src/
│       ├── protocol.cpp
│       └── game_state.cpp
│
├── 📂 assets/              # RESOURCES
│   ├── images/            # Game images/icons
│   ├── sounds/            # Sound effects
│   └── fonts/             # Custom fonts
│
├── 📂 database/            # DATABASE FILES
│   └── battleship.db      # SQLite database (created at runtime)
│
├── 📂 docs/                # DOCUMENTATION
│   └── api/               # API documentation
│
├── 📂 tests/               # UNIT TESTS
│   ├── test_board.cpp
│   ├── test_protocol.cpp
│   └── test_elo.cpp
│
├── 📄 Makefile             # BUILD SYSTEM
├── 📄 README.md            # Project documentation
└── 📄 ỨNG DỤNG GAME...pdf  # Requirements document

build/                      # BUILD OUTPUT (gitignored)
├── common/                # Common object files
├── client/                # Client object files
└── server/                # Server object files

bin/                        # BINARY OUTPUT (gitignored)
├── battleship_client      # Client executable
└── battleship_server      # Server executable
```

---

## 📋 MÔ TẢ CHI TIẾT CÁC FILE

### 🎮 CLIENT MODULE

#### `client/include/ui_manager.h`
**Chức năng:** Header file quản lý toàn bộ giao diện người dùng

**Nội dung chính:**
- Class `UIManager` - Quản lý GTK window và widgets
- Struct `ColorScheme` - Định nghĩa bảng màu (Ocean Blue theme)
- Enum `UIScreen` - Các màn hình game (Login, Lobby, Game, etc.)
- Function declarations cho GTK callbacks

**Đặc điểm giao diện:**
- 🌊 Màu chủ đạo: Ocean Blue (#00B8D4) và White (#FFFFFF)
- ✨ Hiệu ứng glow và pulse theo phong cách Crypto
- 🌊 Animated wave background
- 💫 Smooth transitions và animations

#### `client/src/ui_manager.cpp`
**Chức năng:** Implementation của UI Manager

**Các màn hình:**
1. **Login Screen** - Đăng nhập/đăng ký
2. **Lobby Screen** - Danh sách người chơi, matchmaking
3. **Ship Placement** - Bố trí tàu trước trận
4. **Game Screen** - Màn hình chơi chính với:
   - Board người chơi (trái)
   - Center panel (thông tin trận đấu, buttons, chat)
   - Board đối thủ (phải)
5. **Replay Screen** - Xem lại trận đấu
6. **Profile Screen** - Thông tin cá nhân

**Rendering:**
- `drawBoard()` - Vẽ bàn cờ 10x10
- `drawCell()` - Vẽ từng ô với state (Empty/Ship/Hit/Miss)
- `drawGlowEffect()` - Hiệu ứng phát sáng
- `drawWaveBackground()` - Animation sóng biển

#### `client/src/main.cpp`
**Chức năng:** Entry point của client

**Workflow:**
1. Khởi tạo UIManager
2. Tạo demo boards (sẽ kết nối server sau)
3. Set up callbacks
4. Chạy GTK main loop
5. Cleanup khi thoát

---

### 🖥️ SERVER MODULE (TODO)

Hiện tại server chưa được implement. Kế hoạch:

#### `server/include/server.h`
- Socket server với multithreading
- Quản lý kết nối client
- Message routing

#### `server/include/auth_manager.h`
- Đăng ký/đăng nhập
- Session management
- Password hashing

#### `server/include/match_manager.h`
- Tạo và quản lý trận đấu
- Matchmaking logic
- Turn management

#### `server/include/database.h`
- SQLite wrapper
- User management
- Match history storage
- ELO calculation

---

### 🔗 COMMON MODULE

#### `common/include/protocol.h`
**Chức năng:** Định nghĩa protocol network

**Structures:**
- `MessageHeader` - Header cho mọi message
- `Coordinate` - Tọa độ trên board (row, col)
- `Ship` - Thông tin tàu chiến

**Enums:**
- `MessageType` - Loại message (30+ types)
- `PlayerStatus` - Online/Available/In Game/Busy
- `GameResult` - Win/Loss/Draw
- `ShotResult` - Miss/Hit/Sunk
- `ShipType` - 5 loại tàu
- `Orientation` - Horizontal/Vertical

**Constants:**
- `PROTOCOL_VERSION`, `DEFAULT_PORT`, `BUFFER_SIZE`, etc.

#### `common/include/game_state.h`
**Chức năng:** Logic game và quản lý state

**Classes:**

1. **`Board`**
   - Grid 10x10 với CellState
   - Quản lý 5 tàu chiến
   - Methods:
     - `placeShip()` - Đặt tàu
     - `processShot()` - Xử lý bắn
     - `allShipsSunk()` - Kiểm tra thua
     - `randomPlacement()` - Bố trí ngẫu nhiên

2. **`MatchState`**
   - Thông tin trận đấu đầy đủ
   - 2 boards (player1, player2)
   - Move history
   - ELO before/after
   - Methods:
     - `startMatch()`, `endMatch()`
     - `switchTurn()`
     - `processMove()` - Xử lý nước đi

3. **`PlayerInfo`**
   - User ID, username, display name
   - ELO rating và statistics
   - Status và last seen

---

## 🔧 BUILD SYSTEM

### `Makefile`

**Targets chính:**
```bash
make all           # Build client + server
make client        # Build client only
make server        # Build server only
make clean         # Clean build files
make debug         # Build với debug symbols
make run-client    # Build và chạy client
make run-server    # Build và chạy server
make install-deps  # Cài dependencies
make help          # Hiển thị help
```

**Compiler flags:**
- `-std=c++11` - C++11 standard
- `-Wall -Wextra` - All warnings
- `-O2` - Optimization level 2
- `-g` - Debug symbols (khi dùng `make debug`)

**Libraries:**
- GTK+ 3.0 - GUI framework
- Cairo - 2D graphics
- SQLite3 - Database
- pthread - Multithreading

---

## 📊 DATA FLOW

### 🔄 Client-Server Communication

```
CLIENT                          SERVER
  │                              │
  ├─── AUTH_LOGIN ──────────────>│
  │<─────── AUTH_RESPONSE ───────┤
  │                              │
  ├─── PLAYER_LIST_REQUEST ─────>│
  │<─────── PLAYER_LIST ─────────┤
  │                              │
  ├─── CHALLENGE_SEND ──────────>│
  │                              ├─── Forward to opponent
  │                              │
  │<─────── MATCH_START ─────────┤
  │                              │
  ├─── SHIP_PLACEMENT ──────────>│
  │                              │
  ├─── MOVE ────────────────────>│
  │                              ├─── Validate
  │                              ├─── Process
  │<─────── MOVE_RESULT ─────────┤
  │                              │
  │<─────── MATCH_END ───────────┤
  │<─────── ELO_UPDATE ──────────┤
  │                              │
```

### 🎮 Game Loop

```
1. Login/Register
   ↓
2. Lobby (danh sách người chơi)
   ↓
3. Matchmaking (challenge/accept)
   ↓
4. Ship Placement (90 giây)
   ↓
5. Gameplay Loop:
   - Check turn
   - Player shoots
   - Process result
   - Switch turn (if miss)
   - Check win condition
   ↓
6. Match End
   - Calculate ELO
   - Save to database
   - Show results
   ↓
7. Rematch hoặc về Lobby
```

---

## 🎨 GIAO DIỆN

### Color Palette

```
Primary Colors:
- Ocean Blue:  #00B8D4  rgb(0, 184, 212)
- Dark Blue:   #0097A7  rgb(0, 151, 167)
- White:       #FFFFFF  rgb(255, 255, 255)
- Dark BG:     #0D121F  rgb(13, 18, 31)

Accent Colors:
- Cyan Glow:   #00E5FF  rgb(0, 229, 255)
- Green (Hit): #00E676  rgb(0, 230, 118)
- Red (Miss):  #FF4040  rgb(255, 64, 64)
- Gold:        #FFD700  rgb(255, 215, 0)
```

### Layout

```
┌────────────────────────────────────────────────────┐
│  ⚓ BATTLESHIP ONLINE      ⭐1250  🏆68%  👤Player │ Header
├────────────────────────────────────────────────────┤
│  ┌──────────┐  ┌────────────┐  ┌──────────┐      │
│  │  YOUR    │  │   TIMER    │  │  ENEMY   │      │
│  │  FLEET   │  │   00:45    │  │  WATERS  │      │
│  │          │  │            │  │          │      │
│  │  Board   │  │ YOUR TURN  │  │  Board   │      │ Main
│  │  10x10   │  │            │  │  10x10   │      │
│  │          │  │  Stats     │  │          │      │
│  │          │  │            │  │          │      │
│  │  Ships   │  │  Buttons   │  │ Opponent │      │
│  │  Status  │  │            │  │   Info   │      │
│  │          │  │   Chat     │  │          │      │
│  └──────────┘  └────────────┘  └──────────┘      │
├────────────────────────────────────────────────────┤
│  🎮 v1.0  |  Match #BT-12345  |  📡 45ms         │ Footer
└────────────────────────────────────────────────────┘
```

---

## 📈 DEPENDENCIES GRAPH

```
main.cpp
  └─> ui_manager.h
       ├─> game_state.h
       │    └─> protocol.h
       └─> GTK+ 3.0
            └─> Cairo
```

---

## 🚀 FUTURE ENHANCEMENTS

### Phase 2: Server Implementation
- [ ] Socket server với epoll/select
- [ ] Authentication system
- [ ] Match management
- [ ] Database integration
- [ ] ELO calculation

### Phase 3: Advanced Features
- [ ] Sound effects và music
- [ ] Custom avatars
- [ ] Friend system
- [ ] Tournaments
- [ ] Achievements
- [ ] Leaderboard UI
- [ ] Game variants (Salvo, Fog of War)

### Phase 4: Polish
- [ ] Better animations
- [ ] Mobile responsive (GTK+ mobile)
- [ ] Localization (i18n)
- [ ] Accessibility features
- [ ] Performance optimization

---

## 📝 CODING CONVENTIONS

### Naming
- **Classes**: PascalCase (`UIManager`, `Board`)
- **Functions**: camelCase (`drawBoard()`, `processShot()`)
- **Variables**: snake_case (`player_board`, `current_turn`)
- **Constants**: UPPER_SNAKE_CASE (`BOARD_SIZE`, `CELL_PADDING`)
- **Enums**: PascalCase with prefix (`CELL_EMPTY`, `SHOT_MISS`)

### File Organization
- Header guards: `#ifndef FILENAME_H`
- Include order: System headers → GTK → Local headers
- One class per file
- Implementation (.cpp) separated from declaration (.h)

### Comments
- Doxygen style for public APIs
- Inline comments cho logic phức tạp
- TODO comments cho tính năng sẽ làm

---

**📅 Last Updated:** 2025-10-29
**📌 Version:** 1.0
**👨‍💻 Status:** Client UI Complete, Server TODO
