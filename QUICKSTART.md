# ⚡ QUICKSTART - BATTLESHIP ONLINE

> Bắt đầu chơi game trong 5 phút! 🚀

## 🎯 Bước 1: Cài đặt Dependencies (1 phút)

```bash
cd battleship
make install-deps
```

Đợi Ubuntu/Debian cài đặt GTK+, Cairo, SQLite...

## 🔨 Bước 2: Build Project (1 phút)

```bash
make all
```

Bạn sẽ thấy:
```
╔════════════════════════════════════╗
║  ⚓ BATTLESHIP ONLINE - BUILD ⚓   ║
╚════════════════════════════════════╝
🔧 Compiling...
✅ Client built successfully!
```

## 🎮 Bước 3: Chạy Game (30 giây)

```bash
make run-client
```

Cửa sổ game sẽ mở với giao diện đẹp mắt! 🌊✨

## 🎨 Những gì bạn sẽ thấy

```
┌──────────────────────────────────────┐
│  ⚓ BATTLESHIP ONLINE                │
│                                      │
│  🌊 Giao diện xanh nước biển đẹp    │
│  ✨ Hiệu ứng glow phong cách Crypto │
│  🎯 Bàn cờ 10x10 với animations     │
│  💬 Chat box                         │
│  📊 Thống kê real-time              │
└──────────────────────────────────────┘
```

## 🕹️ Cách chơi (Demo mode)

Hiện tại client chạy ở **demo mode**:

1. **Click Login** → Vào game screen
2. **Click vào cells** trên board đối thủ (bên phải)
3. **Xem animations** đẹp mắt
4. **Thử chat box** (UI sẵn sàng)
5. **Click các buttons**: Fire, Pause, Draw, Resign

## 📝 Lưu ý

- ⚠️ Server chưa hoàn thành → Chưa có multiplayer thực sự
- ✅ Giao diện và animations hoàn chỉnh
- ✅ Game logic sẵn sàng
- ✅ Có thể test UI và hiệu ứng

## 🐛 Lỗi thường gặp?

### Lỗi: "gtk/gtk.h not found"
```bash
sudo apt-get install libgtk-3-dev
```

### Lỗi: Build failed
```bash
make clean && make all
```

### Cửa sổ không hiển thị (WSL)
```bash
export DISPLAY=$(cat /etc/resolv.conf | grep nameserver | awk '{print $2}'):0
# Cần X Server (VcXsrv hoặc X410)
```

## 📚 Muốn tìm hiểu thêm?

- **Chi tiết build**: `BUILD_GUIDE.md`
- **Cấu trúc code**: `PROJECT_STRUCTURE.md`
- **Tổng quan dự án**: `README.md`
- **Tóm tắt**: `SUMMARY.md`

## 🎯 Next Steps

Sau khi chạy demo:

1. ✅ Đánh giá giao diện đẹp
2. ✅ Test các tính năng UI
3. ✅ Xem code để học
4. ⏳ Chờ server được hoàn thành để chơi multiplayer thật!

## 💡 Tips

```bash
# Rebuild nhanh
make client

# Debug mode
make debug
gdb ./bin/battleship_client

# Xem help
make help

# Clean build
make clean && make all
```

## 🎨 Khám phá Features

### Giao diện có gì?
- 🌊 **Animated waves** - Sóng biển chuyển động
- ✨ **Glow effects** - Hiệu ứng phát sáng
- 💎 **Gradient buttons** - Nút bấm đẹp
- 🎯 **Cell hover** - Highlight khi di chuột
- 📊 **Live stats** - Thống kê real-time
- 💬 **Chat UI** - Sẵn sàng chat

### Màu sắc
- **Primary**: Ocean Blue #00B8D4
- **Accent**: Cyan Glow #00E5FF
- **Background**: Dark #0D121F
- **Text**: White #FFFFFF

### Animations
- Pulse effect (60 FPS)
- Wave movement
- Glow pulsing
- Smooth transitions

## 🚀 Ready to Code?

1. Mở VSCode: `code .`
2. Xem `client/src/ui_manager.cpp`
3. Modify và rebuild: `make client`
4. Test ngay: `./bin/battleship_client`

## 🎮 Enjoy!

**Chúc bạn khám phá vui vẻ!** ⚓🌊✨

---

*Tạo issue trên GitHub nếu gặp vấn đề*
*Made with ❤️ and C++*
