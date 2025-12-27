# Battleship Multiplayer - Packet Structure

Tổng quan về cấu trúc gói tin (packet) trong hệ thống Battleship Multiplayer.

---

## 1. Cấu Trúc Packet Tổng Quát

```mermaid
graph LR
    A[Complete Packet] --> B[MessageHeader<br/>77 bytes]
    A --> C[Payload<br/>Variable Size]

    B --> D[type: 1 byte]
    B --> E[length: 4 bytes]
    B --> F[timestamp: 8 bytes]
    B --> G[session_token: 64 bytes]

    C --> H[Message Data<br/>Depends on type]

    style B fill:#90EE90
    style C fill:#FFB6C1
```

**Mô tả**:
- Mỗi packet gồm 2 phần: **Header** (cố định 77 bytes) + **Payload** (kích thước thay đổi)
- Header chứa metadata về message
- Payload chứa dữ liệu thực tế của message

---

## 2. Chi Tiết MessageHeader (77 bytes)

```mermaid
classDiagram
    class MessageHeader {
        +uint8_t type
        +uint32_t length
        +uint64_t timestamp
        +char session_token[64]
    }

    note for MessageHeader "Total: 77 bytes\n1 + 4 + 8 + 64 = 77"
```

**Các trường trong Header**:

| Trường | Kiểu dữ liệu | Kích thước | Mô tả |
|--------|-------------|-----------|-------|
| `type` | `uint8_t` | 1 byte | Loại message (AUTH_LOGIN, SHIP_PLACEMENT, ...) |
| `length` | `uint32_t` | 4 bytes | Kích thước payload (bytes) |
| `timestamp` | `uint64_t` | 8 bytes | Unix timestamp (thời điểm gửi) |
| `session_token` | `char[64]` | 64 bytes | Token xác thực (cho authenticated messages) |

---

## 3. Binary Layout

```mermaid
graph TB
    subgraph "Packet trên Network (Binary)"
        direction LR
        A["Byte 0<br/>type"] --> B["Bytes 1-4<br/>length"]
        B --> C["Bytes 5-12<br/>timestamp"]
        C --> D["Bytes 13-76<br/>session_token"]
        D --> E["Bytes 77+<br/>payload"]
    end

    style A fill:#FF6B6B
    style B fill:#4ECDC4
    style C fill:#45B7D1
    style D fill:#FFA07A
    style E fill:#98D8C8
```

**Ví dụ Login Request**:
```
[Header: 77 bytes]
  type: 0x02 (AUTH_LOGIN)
  length: 128 (sizeof(LoginRequest))
  timestamp: 1700000000
  session_token: [empty - 64 null bytes]

[Payload: 128 bytes]
  username: "alice\0\0..." (64 bytes)
  password: "mypassword\0\0..." (64 bytes)

Total: 77 + 128 = 205 bytes
```

---

## 4. Message Types

```mermaid
graph TD
    subgraph "Authentication (1-10)"
        A1[AUTH_REGISTER = 1]
        A2[AUTH_LOGIN = 2]
        A3[AUTH_LOGOUT = 3]
        A4[AUTH_RESPONSE = 4]
    end

    subgraph "Matchmaking (20-29)"
        B1[MATCHMAKING_REQUEST = 20]
        B2[MATCHMAKING_ACCEPT = 21]
        B3[MATCHMAKING_DECLINE = 22]
    end

    subgraph "Game (30-59)"
        C1[GAME_START = 30]
        C2[SHIP_PLACEMENT = 31]
        C3[MAKE_MOVE = 32]
        C4[MOVE_RESULT = 33]
        C5[GAME_OVER = 34]
    end

    subgraph "Chat (60-79)"
        D1[CHAT_MESSAGE = 60]
        D2[PRIVATE_MESSAGE = 61]
    end

    subgraph "System (100+)"
        E1[PING = 101]
        E2[PONG = 102]
    end
```

---

## 5. Serialization Flow

```mermaid
sequenceDiagram
    participant App as Application
    participant Ser as MessageSerialization
    participant Net as Network Socket

    Note over App,Net: Sending Message
    App->>Ser: Create LoginRequest struct
    Ser->>Ser: serialize(request) → binary string
    Ser->>Ser: Create MessageHeader
    Ser->>Net: send(header) - 77 bytes
    Ser->>Net: send(payload) - length bytes

    Note over App,Net: Receiving Message
    Net->>Ser: recv(header) - 77 bytes
    Ser->>Ser: Parse MessageHeader
    Net->>Ser: recv(payload) - header.length bytes
    Ser->>Ser: deserialize(payload) → struct
    Ser->>App: Deliver message
```

**Serialization Functions**:
```cpp
// Convert struct to binary string
template<typename T>
std::string serialize(const T& data) {
    return std::string(reinterpret_cast<const char*>(&data), sizeof(T));
}

// Convert binary string to struct
template<typename T>
bool deserialize(const std::string& payload, T& data) {
    if (payload.size() != sizeof(T)) return false;
    memcpy(&data, payload.data(), sizeof(T));
    return true;
}
```

---

## 6. Example: Login Request Packet

```mermaid
graph TB
    subgraph "Client Side"
        A[Create LoginRequest] --> B[Set username = 'alice']
        B --> C[Set password = 'pass123']
        C --> D[serialize to binary]
        D --> E[Create MessageHeader<br/>type=AUTH_LOGIN<br/>length=128]
        E --> F[Send header 77 bytes]
        F --> G[Send payload 128 bytes]
    end

    subgraph "Network"
        G --> H[TCP Stream<br/>205 bytes total]
    end

    subgraph "Server Side"
        H --> I[Receive header 77 bytes]
        I --> J[Parse type = AUTH_LOGIN]
        J --> K[Receive payload 128 bytes]
        K --> L[deserialize to LoginRequest]
        L --> M[Process login]
    end

    style A fill:#90EE90
    style M fill:#90EE90
```

**Binary Representation**:
```
Offset | Data                           | Description
-------|--------------------------------|------------------
0x00   | 02                             | type = AUTH_LOGIN
0x01   | 80 00 00 00                    | length = 128 (little-endian)
0x05   | 00 94 65 65 00 00 00 00        | timestamp
0x0D   | 00 00 00 ... (60 more bytes)   | session_token (empty)
0x4D   | 61 6C 69 63 65 00 ... (58)     | username "alice"
0x8D   | 70 61 73 73 31 32 33 00 ... (57) | password "pass123"
```

---

## 7. Payload Size by Message Type

```mermaid
graph LR
    subgraph "Fixed Size Payloads"
        A[RegisterRequest: 192 bytes]
        B[LoginRequest: 128 bytes]
        C[LogoutRequest: 128 bytes]
        D[RegisterResponse: 261 bytes]
        E[LoginResponse: 521 bytes]
        F[LogoutResponse: 1 byte]
    end

    subgraph "Variable Size"
        G[CHAT_MESSAGE: up to 4KB]
        H[SHIP_PLACEMENT: ~200 bytes]
    end

    subgraph "No Payload"
        I[PING: 0 bytes]
        J[PONG: 0 bytes]
    end
```

---

## 8. Important Notes

### Packed Structs
Tất cả message structs sử dụng `__attribute__((packed))`:
```cpp
struct LoginRequest {
    char username[64];
    char password[64];
} __attribute__((packed));
```
- Đảm bảo không có padding bytes
- Kích thước chính xác, nhất quán giữa client/server
- Cross-platform compatibility

### Network Byte Order
- **Header fields**: Giữ nguyên host byte order (hiện tại)
- **TCP**: Đảm bảo thứ tự bytes khi truyền
- **Future**: Có thể cần htonl/ntohl cho multi-platform

### TCP Stream vs Packets
- TCP là **stream-based**, không có packet boundaries
- Sử dụng `length` field để biết khi nào đủ data
- Luôn recv với `MSG_WAITALL` để đảm bảo đủ bytes:
  ```cpp
  recv(socket, &header, sizeof(MessageHeader), MSG_WAITALL);
  recv(socket, payload, header.length, MSG_WAITALL);
  ```

---

**File Location**: `common/include/protocol.h`, `common/include/message_serialization.h`

Last Updated: 2025-11-21
