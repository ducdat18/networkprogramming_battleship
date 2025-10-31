# 🔨 HƯỚNG DẪN BUILD VÀ CHẠY BATTLESHIP ONLINE

## 📋 Mục lục

1. [Yêu cầu hệ thống](#yêu-cầu-hệ-thống)
2. [Cài đặt dependencies](#cài-đặt-dependencies)
3. [Build project](#build-project)
4. [Chạy ứng dụng](#chạy-ứng-dụng)
5. [Troubleshooting](#troubleshooting)
6. [Development Tips](#development-tips)

---

## 🖥️ Yêu cầu hệ thống

### Phần cứng
- **CPU**: Dual-core 1 GHz trở lên
- **RAM**: 512 MB (khuyến nghị 1 GB)
- **GPU**: Hỗ trợ OpenGL 2.0+
- **Storage**: 100 MB dung lượng trống

### Hệ điều hành được hỗ trợ
✅ Ubuntu 18.04 LTS hoặc mới hơn
✅ Debian 10 (Buster) hoặc mới hơn
✅ Linux Mint 19 hoặc mới hơn
✅ Fedora 30 hoặc mới hơn
✅ Arch Linux (latest)

### Phiên bản công cụ
- **GCC/G++**: 7.0 trở lên
- **Make**: 4.0 trở lên
- **pkg-config**: 0.29 trở lên
- **GTK+**: 3.22 trở lên
- **Cairo**: 1.14 trở lên
- **SQLite**: 3.22 trở lên

---

## 📦 Cài đặt dependencies

### Cách 1: Sử dụng Makefile (Khuyến nghị)

```bash
cd battleship
make install-deps
```

Lệnh này sẽ tự động cài đặt tất cả dependencies cần thiết.

### Cách 2: Cài đặt thủ công

#### Ubuntu / Debian

```bash
# Cập nhật package list
sudo apt-get update

# Cài đặt build tools
sudo apt-get install -y build-essential g++ make pkg-config

# Cài đặt GTK+ 3.0 và Cairo
sudo apt-get install -y libgtk-3-dev libcairo2-dev

# Cài đặt SQLite3
sudo apt-get install -y libsqlite3-dev

# Cài đặt Git (nếu chưa có)
sudo apt-get install -y git
```

#### Fedora / Red Hat

```bash
sudo dnf groupinstall "Development Tools"
sudo dnf install gtk3-devel cairo-devel sqlite-devel
```

#### Arch Linux

```bash
sudo pacman -S base-devel gtk3 cairo sqlite
```

### Kiểm tra cài đặt

```bash
# Kiểm tra GCC
g++ --version
# Expected: g++ (Ubuntu X.X.X) 7.0.0 hoặc cao hơn

# Kiểm tra GTK+
pkg-config --modversion gtk+-3.0
# Expected: 3.22.0 hoặc cao hơn

# Kiểm tra Cairo
pkg-config --modversion cairo
# Expected: 1.14.0 hoặc cao hơn

# Kiểm tra SQLite
sqlite3 --version
# Expected: 3.22.0 hoặc cao hơn
```

---

## 🔨 Build project

### Build toàn bộ (Client + Server)

```bash
cd battleship
make all
```

**Output:**
```
╔════════════════════════════════════╗
║  ⚓ BATTLESHIP ONLINE - BUILD ⚓   ║
╠════════════════════════════════════╣
║  Version: 1.0                      ║
║  Platform: Linux/Ubuntu            ║
║  Language: C++11                   ║
╚════════════════════════════════════╝

🔧 Compiling common/src/protocol.cpp...
🔧 Compiling common/src/game_state.cpp...
🔧 Compiling client/src/main.cpp...
🔧 Compiling client/src/ui_manager.cpp...
🔗 Linking client...
✅ Client built successfully!
╔════════════════════════════════════╗
║   BUILD COMPLETED SUCCESSFULLY!    ║
╚════════════════════════════════════╝
Client binary: bin/battleship_client
Server binary: bin/battleship_server
```

### Build riêng lẻ

#### Chỉ build Client

```bash
make client
```

Tạo file: `bin/battleship_client`

#### Chỉ build Server

```bash
make server
```

Tạo file: `bin/battleship_server`

### Build Debug version

```bash
make debug
```

Thêm debug symbols (-g flag) và định nghĩa DEBUG macro.

### Clean build files

```bash
make clean
```

Xóa tất cả file trong thư mục `build/` và `bin/`.

### Rebuild từ đầu

```bash
make clean && make all
```

---

## 🚀 Chạy ứng dụng

### Chạy Client

#### Cách 1: Sử dụng Makefile

```bash
make run-client
```

#### Cách 2: Chạy trực tiếp

```bash
./bin/battleship_client
```

**Output mong đợi:**
```
==================================
⚓ BATTLESHIP ONLINE - CLIENT ⚓
==================================
Version: 1.0
Platform: Linux/Ubuntu
Graphics: GTK+ 3.0
==================================

✅ UI initialized successfully!
🚀 Starting game...
```

Sau đó cửa sổ GTK sẽ mở lên với giao diện game.

### Chạy Server (TODO)

```bash
make run-server
# hoặc
./bin/battleship_server
```

**Lưu ý:** Server hiện chưa được implement đầy đủ.

### Chạy cùng lúc Client và Server

Mở 2 terminal:

**Terminal 1 - Server:**
```bash
cd battleship
make run-server
```

**Terminal 2 - Client:**
```bash
cd battleship
make run-client
```

---

## 🐛 Troubleshooting

### Lỗi: "gtk/gtk.h: No such file or directory"

**Nguyên nhân:** Chưa cài GTK+ development headers

**Giải pháp:**
```bash
sudo apt-get install libgtk-3-dev
```

### Lỗi: "Package gtk+-3.0 was not found"

**Nguyên nhân:** pkg-config không tìm thấy GTK+

**Giải pháp:**
```bash
# Kiểm tra đường dẫn pkg-config
export PKG_CONFIG_PATH=/usr/lib/pkgconfig:/usr/local/lib/pkgconfig

# Cài lại GTK+
sudo apt-get install --reinstall libgtk-3-dev
```

### Lỗi: "undefined reference to `gtk_init'"

**Nguyên nhân:** Thiếu linker flags cho GTK+

**Giải pháp:** Makefile đã handle việc này. Nếu vẫn lỗi:
```bash
# Kiểm tra GTK libs
pkg-config --libs gtk+-3.0

# Build lại
make clean && make all
```

### Lỗi: "error: 'M_PI' was not declared"

**Nguyên nhân:** Thiếu define _USE_MATH_DEFINES hoặc include <cmath>

**Giải pháp:** Đã được fix trong code (include <cmath>)

### Lỗi: "Segmentation fault"

**Nguyên nhân:** Null pointer hoặc memory access violation

**Giải pháp debug:**
```bash
# Build debug version
make debug

# Chạy với GDB
gdb ./bin/battleship_client
(gdb) run
# Khi crash:
(gdb) backtrace
(gdb) info locals

# Hoặc dùng Valgrind
valgrind --leak-check=full --track-origins=yes ./bin/battleship_client
```

### Cửa sổ GTK không hiển thị

**Nguyên nhân:** Display environment chưa được set

**Giải pháp:**
```bash
# Kiểm tra DISPLAY
echo $DISPLAY
# Nếu rỗng:
export DISPLAY=:0

# Nếu dùng WSL:
export DISPLAY=$(cat /etc/resolv.conf | grep nameserver | awk '{print $2}'):0
```

### Hiệu ứng animation bị lag

**Nguyên nhân:** GPU acceleration không được bật

**Giải pháp:**
```bash
# Kiểm tra OpenGL
glxinfo | grep "OpenGL version"

# Bật GPU acceleration cho GTK+
export GDK_RENDERING=gl
```

---

## 💡 Development Tips

### Hot Reload Development

Sử dụng script để tự động build khi file thay đổi:

```bash
# Install inotify-tools
sudo apt-get install inotify-tools

# Watch và auto-rebuild
while inotifywait -e modify client/src/*.cpp client/include/*.h; do
    make client
done
```

### Debug Rendering

Thêm debug output trong `ui_manager.cpp`:

```cpp
void UIManager::drawCell(...) {
    std::cout << "Drawing cell at (" << row << ", " << col << ")" << std::endl;
    // ... rest of code
}
```

### Profile Performance

```bash
# Build với profiling
g++ -pg -o battleship_client ...

# Chạy
./battleship_client

# Phân tích
gprof battleship_client gmon.out > analysis.txt
```

### Memory Leak Detection

```bash
# Chạy với Valgrind
valgrind --leak-check=full \
         --show-leak-kinds=all \
         --track-origins=yes \
         --verbose \
         --log-file=valgrind-out.txt \
         ./bin/battleship_client
```

### Code Formatting

```bash
# Cài đặt clang-format
sudo apt-get install clang-format

# Format tất cả file
find client common -name "*.cpp" -o -name "*.h" | xargs clang-format -i
```

---

## 📊 Build Optimization

### Release Build (Optimized)

```bash
# Sửa Makefile để thêm -O3
CXXFLAGS = -std=c++11 -Wall -Wextra -O3 -march=native

make clean && make all
```

### Static Linking (Portable binary)

```bash
# Build với static libraries
g++ -static -o battleship_client \
    $(OBJECTS) \
    $(shell pkg-config --libs --static gtk+-3.0)
```

**Lưu ý:** Binary sẽ lớn hơn (~50MB) nhưng không cần cài GTK+ trên máy khác.

### Strip Binary (Giảm kích thước)

```bash
# Loại bỏ debug symbols
strip bin/battleship_client

# Kích thước giảm ~30-40%
```

---

## 🧪 Testing

### Unit Tests (TODO)

```bash
# Build tests
make tests

# Run tests
./bin/run_tests
```

### Integration Tests

```bash
# Test client kết nối server
./bin/battleship_server &
sleep 2
./bin/battleship_client
```

---

## 📦 Packaging

### Create .deb package (Ubuntu/Debian)

```bash
# Cài đặt dpkg-deb
sudo apt-get install dpkg-dev

# Tạo structure
mkdir -p battleship-deb/DEBIAN
mkdir -p battleship-deb/usr/bin
mkdir -p battleship-deb/usr/share/applications

# Copy files
cp bin/battleship_client battleship-deb/usr/bin/
cp bin/battleship_server battleship-deb/usr/bin/

# Tạo control file
cat > battleship-deb/DEBIAN/control << EOF
Package: battleship-online
Version: 1.0
Section: games
Priority: optional
Architecture: amd64
Depends: libgtk-3-0, libcairo2, libsqlite3-0
Maintainer: Your Name <your.email@example.com>
Description: Battleship Online Game
 A multiplayer naval combat game with beautiful graphics
EOF

# Build package
dpkg-deb --build battleship-deb

# Install
sudo dpkg -i battleship-deb.deb
```

---

## 🔍 Makefile Targets Reference

| Target | Mô tả |
|--------|-------|
| `make all` | Build client + server (default) |
| `make client` | Build client only |
| `make server` | Build server only |
| `make debug` | Build with debug symbols |
| `make clean` | Remove build files |
| `make run-client` | Build và chạy client |
| `make run-server` | Build và chạy server |
| `make install-deps` | Cài đặt dependencies |
| `make help` | Hiển thị help |

---

## 📞 Support

Nếu gặp vấn đề:

1. Kiểm tra lại [Troubleshooting](#troubleshooting)
2. Xem log output chi tiết
3. Mở issue trên GitHub với thông tin:
   - OS version: `cat /etc/os-release`
   - GCC version: `g++ --version`
   - GTK version: `pkg-config --modversion gtk+-3.0`
   - Error message đầy đủ

---

**🎯 Chúc bạn build thành công!**

*Last updated: 2025-10-29*
