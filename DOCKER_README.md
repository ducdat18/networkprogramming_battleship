# Docker Setup - Battleship Game Server

## Quick Start

### 1. Build và chạy server
```bash
# Build image
docker-compose build

# Start server
docker-compose up -d

# Xem logs
docker-compose logs -f server

# Stop server
docker-compose down
```

### 2. Development Mode
```bash
# Start dev server (auto-rebuild)
docker-compose --profile dev up server-dev

# Rebuild khi code thay đổi
docker-compose --profile dev up --build server-dev
```

## Commands

### Docker Compose

```bash
# Build lại image
docker-compose build

# Start services
docker-compose up              # Foreground
docker-compose up -d           # Background (detached)

# Stop services
docker-compose down            # Stop và remove containers
docker-compose down -v         # Stop và remove volumes (xóa database!)

# Xem logs
docker-compose logs server          # Tất cả logs
docker-compose logs -f server       # Follow logs (real-time)
docker-compose logs --tail=50 server # 50 dòng cuối

# Restart
docker-compose restart server

# Exec vào container
docker-compose exec server /bin/bash

# Xem resource usage
docker-compose stats
```

### Docker Direct

```bash
# Build image manually
docker build -t battleship-server .

# Run container manually
docker run -d \
  --name battleship_server \
  -p 8888:8888 \
  -v $(pwd)/data:/app/data \
  -v $(pwd)/logs:/app/logs \
  battleship-server

# Stop
docker stop battleship_server

# Remove
docker rm battleship_server

# Exec
docker exec -it battleship_server /bin/bash

# Logs
docker logs -f battleship_server
```

## Testing

### Test kết nối với netcat
```bash
# Từ host machine
nc localhost 8888

# Hoặc telnet
telnet localhost 8888

# Test PING/PONG
echo "PING" | nc localhost 8888
```

### Test từ bên trong container
```bash
# Exec vào container
docker-compose exec server /bin/bash

# Test local connection
nc localhost 8888
```

### Test với multiple clients
```bash
# Terminal 1
nc localhost 8888

# Terminal 2
nc localhost 8888

# Terminal 3
nc localhost 8888
```

## Volumes

Dữ liệu được lưu trong các volumes:

```bash
./data/         # SQLite database
./logs/         # Server logs
```

### Backup database
```bash
# Copy database từ container
docker cp battleship_server:/app/data/battleship.db ./backup/

# Restore database
docker cp ./backup/battleship.db battleship_server:/app/data/
```

## Networking

### Ports
- `8888` - Game server port (TCP)

### Kiểm tra network
```bash
# Xem network details
docker network inspect networkprogramming_battleship_battleship_network

# Xem port mapping
docker port battleship_server
```

## Debugging

### Check server status
```bash
# Health check
docker-compose ps

# Detailed inspect
docker inspect battleship_server
```

### Debug connection issues
```bash
# Kiểm tra port có open không
netstat -tuln | grep 8888

# Kiểm tra firewall (Ubuntu)
sudo ufw status

# Test từ container khác
docker run -it --network networkprogramming_battleship_battleship_network alpine sh
nc battleship_server 8888
```

### Memory/CPU monitoring
```bash
# Real-time stats
docker stats battleship_server

# Detailed resource usage
docker-compose top
```

## Production Deployment

### Build optimized image
```bash
docker build -t battleship-server:1.0 .
```

### Run with resource limits
```bash
docker run -d \
  --name battleship_server \
  --restart unless-stopped \
  --memory="512m" \
  --cpus="2.0" \
  -p 8888:8888 \
  -v /path/to/data:/app/data \
  battleship-server:1.0
```

### Environment variables
```bash
docker run -d \
  --name battleship_server \
  -p 8888:8888 \
  -e SERVER_PORT=8888 \
  -e DB_PATH=/app/data/battleship.db \
  -e LOG_LEVEL=INFO \
  -e MAX_CONNECTIONS=100 \
  battleship-server:1.0
```

## Troubleshooting

### Container không start
```bash
# Xem logs để biết lỗi
docker-compose logs server

# Check exit code
docker-compose ps
```

### Permission issues
```bash
# Fix permissions for data directory
sudo chown -R $USER:$USER data/ logs/
chmod -R 755 data/ logs/
```

### Port already in use
```bash
# Tìm process đang dùng port 8888
sudo lsof -i :8888

# Kill process
sudo kill -9 <PID>

# Hoặc đổi port trong docker-compose.yml
# "8889:8888"  # Host:Container
```

### Database locked
```bash
# Stop tất cả containers
docker-compose down

# Xóa database lock file
rm data/battleship.db-shm data/battleship.db-wal

# Restart
docker-compose up -d
```

## Clean Up

### Remove everything
```bash
# Stop và remove containers, networks
docker-compose down

# Remove images
docker rmi battleship-server

# Remove volumes (CAUTION: Deletes database!)
docker-compose down -v

# Remove all unused Docker resources
docker system prune -a
```

### Keep data, remove containers
```bash
# Stop and remove containers only
docker-compose down

# Data in ./data/ and ./logs/ is preserved
```

## Multi-stage Build

Image sử dụng multi-stage build để giảm size:

**Builder stage** (~600MB):
- Ubuntu + build tools
- Compile source code

**Runtime stage** (~100MB):
- Ubuntu minimal
- SQLite runtime only
- Binary đã compile

Kiểm tra image size:
```bash
docker images battleship-server
```
