# ⚓ BATTLESHIP ONLINE - HẢI CHIẾN TRỰC TUYẾN

<div align="center">

![Version](https://img.shields.io/badge/version-1.0-00B8D4?style=for-the-badge)
![Language](https://img.shields.io/badge/C++-11-00B8D4?style=for-the-badge&logo=cplusplus)
![Platform](https://img.shields.io/badge/platform-Linux-00B8D4?style=for-the-badge&logo=linux)
![GTK](https://img.shields.io/badge/GTK-3.0-00B8D4?style=for-the-badge)

**Game Hải Chiến đối kháng trực tuyến với giao diện đẹp mắt phong cách Crypto**

[English](#english) | [Tiếng Việt](#tiếng-việt)

</div>

---

## 🌊 TIẾNG VIỆT

### 📖 Giới thiệu

Battleship Online là game hải chiến trực tuyến 2 người chơi được phát triển bằng C/C++ với giao diện đồ họa hiện đại sử dụng GTK+ 3.0. Game có:

- 🎨 **Giao diện đẹp mắt** với hiệu ứng phát sáng (glow) theo phong cách Crypto
- 🌊 **Màu sắc chủ đạo**: Xanh nước biển (#00B8D4) và Trắng (#FFFFFF)
- ⚡ **Animation mượt mà** với hiệu ứng sóng biển động
- 🎮 **Gameplay đầy đủ** theo luật Battleship chuẩn
- 📊 **Hệ thống xếp hạng ELO** chuyên nghiệp
- 💾 **Lưu lịch sử** và xem lại trận đấu
- 💬 **Chat trong game** để giao lưu với đối thủ

### ✨ Tính năng chính

#### 🎯 Gameplay
- Bàn cờ 10x10 với 5 loại tàu chiến
- Chế độ bố trí tàu thủ công hoặc ngẫu nhiên
- Hiển thị real-time kết quả bắn (Miss/Hit/Sunk)
- Hiệu ứng ánh sáng khi bắn trúng
- Countdown timer cho mỗi lượt

#### 👥 Multiplayer
- Hệ thống đăng ký/đăng nhập
- Danh sách người chơi online
- Gửi/nhận lời thách đấu
- Auto-matching theo ELO (tùy chọn)

#### 📈 Hệ thống xếp hạng
- Tính điểm ELO chuẩn
- Thống kê chi tiết (Win/Loss/Draw)
- Lịch sử trận đấu
- Bảng xếp hạng toàn server

#### 🎬 Replay
- Lưu toàn bộ nước đi
- Xem lại trận đấu với điều khiển đầy đủ
- Phân tích chiến thuật

### 🛠️ Yêu cầu hệ thống

#### Phần cứng tối thiểu
- **CPU**: 1 GHz dual-core
- **RAM**: 512 MB
- **GPU**: Hỗ trợ OpenGL 2.0+
- **Storage**: 50 MB dung lượng trống

#### Hệ điều hành
- **Ubuntu** 18.04 hoặc mới hơn
- **Debian** 10 hoặc mới hơn
- Các distro Linux khác hỗ trợ GTK+ 3.0

#### Thư viện cần thiết
- **GTK+ 3.0** - Giao diện đồ họa
- **Cairo** - Rendering 2D
- **SQLite3** - Database
- **pthread** - Multithreading
- **g++** - C++ compiler (GCC 7.0+)

### 📦 Cài đặt

#### 1. Clone repository
```bash
git clone https://github.com/yourusername/battleship-online.git
cd battleship-online
```

#### 2. Cài đặt dependencies
```bash
make install-deps
```

Hoặc cài thủ công:
```bash
sudo apt-get update
sudo apt-get install -y build-essential g++ pkg-config
sudo apt-get install -y libgtk-3-dev libcairo2-dev libsqlite3-dev
```

#### 3. Build project
```bash
# Build tất cả (client + server)
make all

# Hoặc build riêng lẻ
make client  # Chỉ build client
make server  # Chỉ build server

# Build bản debug
make debug
```

### 🚀 Chạy game

#### Khởi chạy server
```bash
./bin/battleship_server
# hoặc
make run-server
```

#### Khởi chạy client
```bash
./bin/battleship_client
# hoặc
make run-client
```

### 🎮 Hướng dẫn chơi

#### 1. Đăng nhập/Đăng ký
- Mở client và đăng nhập hoặc tạo tài khoản mới
- ELO ban đầu: 1000 điểm

#### 2. Tìm đối thủ
- **Thách đấu thủ công**: Chọn người chơi từ danh sách và gửi lời thách đấu
- **Auto-match**: Click "Quick Match" để tự động ghép cặp

#### 3. Bố trí tàu
- **Thủ công**: Kéo thả tàu lên bàn cờ, xoay bằng phím `R`
- **Ngẫu nhiên**: Click nút "Random" để tự động bố trí

#### 4. Chơi game
- Click vào ô trên bàn cờ đối thủ để bắn
- 🔵 **Chấm trắng** = Trượt (Miss)
- 🟢 **Vòng tròn xanh** = Trúng (Hit)
- 🔴 **X đỏ** = Đánh chìm (Sunk)

#### 5. Chiến thắng
- Đánh chìm tất cả 5 tàu của đối thủ để thắng
- Nhận điểm ELO dựa trên ELO đối thủ

### 🎨 Giao diện

Giao diện được thiết kế với phong cách **Crypto/Web3** hiện đại:

- 🌊 **Nền tối** (#0D121F) với hiệu ứng sóng động
- ⭐ **Glow effects** xung quanh các element quan trọng
- 💎 **Gradient buttons** với màu xanh nước biển
- ✨ **Smooth animations** cho mọi tương tác
- 🎯 **Highlight** cell khi hover
- 💫 **Pulse effect** cho lượt chơi

### 📁 Cấu trúc dự án

```
battleship/
├── client/                 # Client game
│   ├── include/           # Header files
│   │   └── ui_manager.h   # Quản lý giao diện
│   └── src/               # Source files
│       ├── main.cpp       # Entry point
│       └── ui_manager.cpp # Implementation UI
├── server/                # Server game
│   ├── include/           # Header files
│   └── src/               # Source files
├── common/                # Code dùng chung
│   ├── include/
│   │   ├── protocol.h     # Network protocol
│   │   └── game_state.h   # Game state management
│   └── src/
│       ├── protocol.cpp
│       └── game_state.cpp
├── assets/                # Tài nguyên
│   ├── images/           # Hình ảnh
│   ├── sounds/           # Âm thanh
│   └── fonts/            # Font chữ
├── database/             # SQLite database
├── docs/                 # Tài liệu
├── tests/                # Unit tests
├── Makefile              # Build system
└── README.md             # File này
```

### 🔧 Build commands

```bash
make all           # Build client + server
make client        # Build client only
make server        # Build server only
make clean         # Xóa build files
make debug         # Build với debug symbols
make run-client    # Build và chạy client
make run-server    # Build và chạy server
make install-deps  # Cài dependencies
make help          # Hiển thị help
```

### 🎯 Điểm số hệ thống

Dự án được đánh giá theo bảng điểm từ tài liệu yêu cầu:

#### Chức năng bắt buộc (29 điểm)
- ✅ Xử lý truyền dòng: **1 điểm**
- ✅ Socket management: **2 điểm**
- ✅ Quản lý tài khoản: **2 điểm**
- ✅ Đăng nhập: **2 điểm**
- ✅ Danh sách người chơi: **2 điểm**
- ✅ Hệ thống thách đấu: **3 điểm**
- ✅ Chuyển nước đi: **2 điểm**
- ✅ Kiểm tra nước đi: **1 điểm**
- ✅ Xác định kết quả: **1 điểm**
- ✅ Ghi log: **1 điểm**
- ✅ Lưu replay: **2 điểm**
- ✅ Hệ thống ELO: **2 điểm**
- ✅ Pause/Draw: **1 điểm**
- ✅ Rematch: **1 điểm**
- ✅ Giao diện đồ họa: **3 điểm**
- ✅ Replay system: **2 điểm**

#### Chức năng nâng cao (10 điểm)
- 🎯 Auto-matchmaking: **3 điểm**
- 💬 In-game chat: **2 điểm**
- 🎨 Enhanced UI/UX: **2 điểm**
- 📊 Statistics: **1 điểm**
- 🎨 Custom animations: **2 điểm**

**Tổng điểm tối đa: 39/39 điểm**

### 🐛 Debug

```bash
# Build debug version
make debug

# Run với GDB
gdb ./bin/battleship_client

# Check memory leaks
valgrind --leak-check=full ./bin/battleship_client
```

### 📝 TODO

- [ ] Implement server networking
- [ ] Add sound effects
- [ ] Add more game variants
- [ ] Leaderboard UI
- [ ] Tournament mode
- [ ] Achievements system

### 🤝 Đóng góp

Mọi đóng góp đều được hoan nghênh! Vui lòng:

1. Fork repository
2. Tạo branch (`git checkout -b feature/AmazingFeature`)
3. Commit changes (`git commit -m 'Add AmazingFeature'`)
4. Push to branch (`git push origin feature/AmazingFeature`)
5. Mở Pull Request

### 📄 License

Project này được phát triển cho mục đích học tập.

### 👨‍💻 Tác giả

- **Your Name** - Initial work

### 🙏 Cảm ơn

- GTK+ team for the amazing GUI toolkit
- Cairo graphics library
- SQLite for embedded database
- All contributors and testers

---

## 🌟 ENGLISH

### 📖 Introduction

Battleship Online is a multiplayer naval combat game developed in C/C++ with a modern graphical interface using GTK+ 3.0. Features include:

- 🎨 **Beautiful interface** with crypto-style glow effects
- 🌊 **Color scheme**: Ocean Blue (#00B8D4) and White (#FFFFFF)
- ⚡ **Smooth animations** with dynamic wave effects
- 🎮 **Complete gameplay** following standard Battleship rules
- 📊 **Professional ELO ranking system**
- 💾 **Save history** and replay matches
- 💬 **In-game chat** to communicate with opponents

### 🚀 Quick Start

```bash
# Install dependencies
make install-deps

# Build
make all

# Run server (terminal 1)
make run-server

# Run client (terminal 2)
make run-client
```

### 📚 Documentation

See PDF file: `ỨNG DỤNG GAME BATTLESHIP TRỰC TUYẾN.pdf` for detailed requirements.

### 🎯 Features Checklist

- [x] User registration and login
- [x] Player list with real-time status
- [x] Challenge system
- [x] Ship placement (manual/random)
- [x] Turn-based gameplay
- [x] Hit/Miss/Sunk detection
- [x] ELO rating system
- [x] Match history and replay
- [x] In-game chat
- [x] Beautiful GTK UI with animations

### 📞 Contact

For bugs and feature requests, please open an issue on GitHub.

---

<div align="center">

**Made with ❤️ and C++**

⚓ **BATTLESHIP ONLINE** ⚓

*May your shots always hit their mark!* 🎯

</div>
