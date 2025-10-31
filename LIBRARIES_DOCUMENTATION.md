# 📚 Battleship Online - Thư viện C/C++ sử dụng

## 🔧 Core Technologies

### 1. **GTK+ 3.0** (GNOME Toolkit)
**Thư viện:** `libgtk-3.so`, `libgdk-3.so`
**Mục đích:** GUI Framework chính

**Tính năng sử dụng:**
- Window management (`GtkWindow`)
- Widget containers (`GtkBox`, `GtkScrolledWindow`)
- Buttons (`GtkButton`)
- Text input (`GtkEntry`)
- Labels (`GtkLabel`)
- Drawing areas (`GtkDrawingArea`)

**Ưu điểm:**
- ✅ Cross-platform (Linux, Windows, macOS)
- ✅ Mature, stable, well-documented
- ✅ Native look on Linux
- ✅ Good performance
- ✅ Large community

**Installation:**
```bash
sudo apt-get install libgtk-3-dev
```

**Docs:** https://docs.gtk.org/gtk3/

---

### 2. **Cairo**
**Thư viện:** `libcairo.so`, `libcairo-gobject.so`
**Mục đích:** 2D Graphics rendering

**Tính năng sử dụng:**
- Custom shape drawing (rectangles, circles)
- Gradients và patterns
- Ship rendering (hull, deck, turrets)
- Explosion và water splash effects
- Wave backgrounds

**API sử dụng:**
```c
cairo_rectangle()      // Vẽ hình chữ nhật
cairo_arc()            // Vẽ hình tròn
cairo_pattern_create_*() // Tạo gradient
cairo_set_source_rgb() // Đặt màu
cairo_fill()           // Tô màu
cairo_stroke()         // Vẽ đường viền
```

**Ưu điểm:**
- ✅ Vector graphics (smooth at any scale)
- ✅ Hardware accelerated
- ✅ Beautiful gradients và effects
- ✅ Precise rendering

**Docs:** https://www.cairographics.org/manual/

---

### 3. **GLib 2.0**
**Thư viện:** `libglib-2.0.so`, `libgobject-2.0.so`, `libgmodule-2.0.so`
**Mục đích:** Core utilities và object system

**Tính năng sử dụng:**
- Event loop (`g_main_loop`, `gtk_main`)
- Timers (`g_timeout_add`)
- Signal handling (`g_signal_connect`)
- Memory management
- String utilities
- Data structures

**Ưu điểm:**
- ✅ Foundation cho GTK
- ✅ Cross-platform utilities
- ✅ Efficient event handling

---

### 4. **Pango**
**Thư viện:** `libpangocairo-1.0.so`, `libpango-1.0.so`
**Mục đích:** Text rendering và layout

**Tính năng sử dụng:**
- Font rendering
- Text layout
- Unicode support (emoji, special characters)
- Text measurements

**Ưu điểm:**
- ✅ Beautiful text rendering
- ✅ Full Unicode support
- ✅ International text

---

### 5. **X11 Libraries**
**Thư viện:** `libX11.so`, `libXi.so`, `libXfixes.so`
**Mục đích:** Linux window system

**Tính năng:**
- Window management
- Mouse và keyboard input
- Display handling

---

### 6. **Standard C++ Libraries**

#### **libstdc++** (C++ Standard Library)
**Tính năng sử dụng:**
- `std::string` - String handling
- `std::vector` - Dynamic arrays
- `std::function` - Callbacks
- `std::iostream` - Console I/O

#### **libm** (Math Library)
**Tính năng sử dụng:**
- `sin()`, `cos()` - Wave animations
- `fmod()` - Modulo for animations
- `M_PI` - Circle calculations

#### **libpthread** (POSIX Threads)
**Sẵn sàng cho:**
- Network thread
- Background tasks
- Async operations

---

## 📦 Cấu trúc thư viện trong Project

### Build System: **GNU Make**
```makefile
CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -O2

GTK_CFLAGS = $(shell pkg-config --cflags gtk+-3.0 cairo)
GTK_LIBS = $(shell pkg-config --libs gtk+-3.0 cairo)
```

### Compiler: **GCC/G++**
- Version: C++11 standard
- Optimization: -O2
- Warnings: -Wall -Wextra

---

## 🎯 Thư viện sẽ dùng cho các tính năng tiếp theo

### Networking (TODO)
```cpp
// Linux socket API
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

// Hoặc dùng boost::asio
// #include <boost/asio.hpp>
```

### Database (TODO)
```cpp
// SQLite3
#include <sqlite3.h>

// Install:
// sudo apt-get install libsqlite3-dev
```

### Sound (TODO - Optional)
```cpp
// SDL2_mixer
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>

// Install:
// sudo apt-get install libsdl2-dev libsdl2-mixer-dev

// Hoặc OpenAL
#include <AL/al.h>
#include <AL/alc.h>
```

---

## 📊 Dependencies Graph

```
battleship_client
    ├── GTK+ 3.0 (GUI Framework)
    │   ├── GDK (Drawing Kit)
    │   ├── GLib (Core utilities)
    │   └── GObject (Object system)
    ├── Cairo (2D Graphics)
    │   └── libcairo-gobject
    ├── Pango (Text rendering)
    │   └── libpangocairo
    ├── X11 (Window system)
    │   ├── libXi (Input)
    │   └── libXfixes
    ├── libstdc++ (C++ stdlib)
    ├── libm (Math)
    └── libpthread (Threads)

battleship_server (TODO)
    ├── C++ stdlib
    ├── POSIX sockets
    ├── SQLite3
    └── pthread
```

---

## 💾 Kích thước thư viện

```
libgtk-3.so.0       ~18 MB   (Main GUI)
libcairo.so.2       ~600 KB  (Graphics)
libglib-2.0.so.0    ~1.2 MB  (Core utils)
libstdc++.so.6      ~2 MB    (C++ stdlib)
```

**Tổng:** Khoảng 30-40 MB dependencies

---

## 🔍 Kiểm tra dependencies

```bash
# Check linked libraries
ldd ./bin/battleship_client

# Check if libraries installed
pkg-config --list-all | grep gtk
pkg-config --modversion gtk+-3.0
pkg-config --modversion cairo

# Check library paths
ldconfig -p | grep gtk
```

---

## 🚀 Build từ source

### Install dependencies Ubuntu/Debian:
```bash
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    g++ \
    make \
    pkg-config \
    libgtk-3-dev \
    libcairo2-dev \
    libsqlite3-dev
```

### Build:
```bash
make clean
make client
make server  # TODO: Chưa xong
```

---

## 📖 Learning Resources

### GTK+ 3
- Official Docs: https://docs.gtk.org/gtk3/
- Tutorial: https://www.gtk.org/docs/getting-started/
- Examples: /usr/share/doc/libgtk-3-dev/examples/

### Cairo
- Manual: https://www.cairographics.org/manual/
- Tutorial: https://www.cairographics.org/tutorial/
- Samples: https://www.cairographics.org/samples/

### C++11
- cppreference: https://en.cppreference.com/
- C++11 features: https://isocpp.org/

---

## ⚡ Performance Notes

### GTK+ 3
- Event-driven architecture (efficient)
- Hardware-accelerated rendering
- Minimal CPU usage when idle
- ~10-20 MB RAM usage

### Cairo
- Vector graphics (scales perfectly)
- Efficient gradient rendering
- Can use OpenGL backend
- ~5-10 MB RAM for surfaces

### Game performance
- 60 FPS achievable
- ~30-50 MB RAM total
- Low CPU (<5% idle, <15% active)

---

## 🎨 Tại sao chọn GTK+/Cairo?

### ✅ Ưu điểm
1. **Native Linux support** - Chạy tốt nhất trên Linux
2. **Cross-platform** - Có thể port sang Windows/macOS
3. **Mature ecosystem** - Ổn định, nhiều tài liệu
4. **Beautiful graphics** - Cairo rendering đẹp
5. **Easy to learn** - API rõ ràng, logic
6. **Free & Open Source** - LGPL license

### ❌ Nhược điểm
1. **Learning curve** - GTK có nhiều concepts
2. **Windows support** - Cần MSYS2/MinGW
3. **Not gaming-focused** - Không phải game engine

### 🆚 So sánh alternatives

| Library | Pros | Cons |
|---------|------|------|
| **GTK+** | Native Linux, mature | Not game-focused |
| Qt | Cross-platform, rich | Heavy, C++ complex |
| SDL2 | Game-focused, fast | Low-level, no GUI |
| wxWidgets | Native look | Old API style |
| Dear ImGui | Gaming UI, easy | C++ only, simple UI |

**Kết luận:** GTK+ là lựa chọn tốt cho game 2D strategy như Battleship!

---

## 🔮 Future Libraries (Planned)

### Phase 3 - Networking
- **POSIX Sockets** hoặc **Boost.Asio**
- Async I/O
- Protocol serialization

### Phase 4 - Database
- **SQLite3** - Embedded database
- User accounts, ELO, history

### Phase 5 - Audio
- **SDL2_mixer** hoặc **OpenAL**
- Sound effects, background music

---

Made with ⚓
