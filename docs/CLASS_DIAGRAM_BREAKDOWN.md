# Battleship Multiplayer - Class Diagram Breakdown

This document provides a detailed breakdown of each component in the class diagram with Mermaid diagrams for each section.

---

## Table of Contents

1. [Client-Side Components](#1-client-side-components)
2. [Server-Side Components](#2-server-side-components)
3. [Common/Shared Components](#3-commonshared-components)
4. [Game Logic Components](#4-game-logic-components)
5. [Authentication Messages](#5-authentication-messages)
6. [Database Models](#6-database-models)
7. [Message Flow Diagrams](#7-message-flow-diagrams)

---

## 1. Client-Side Components

```mermaid
classDiagram
    class UIManager {
        -GtkWidget* window
        -GtkWidget* stack
        -ClientNetwork* network
        -SessionStorage* session_storage
        -AssetManager* asset_manager
        -Player current_player
        -vector~Player~ online_players
        -map~string,GtkWidget*~ screens
        +UIManager()
        +initialize() bool
        +run() void
        +showScreen(ScreenType) void
        +handleLoginResponse(LoginResponse*) void
        +handleRegisterResponse(RegisterResponse*) void
        +handleLogoutResponse() void
        +updatePlayerList(vector~Player~) void
        -createLoginScreen() GtkWidget*
        -createRegisterScreen() GtkWidget*
        -createMainMenuScreen() GtkWidget*
        -createLobbyScreen() GtkWidget*
        -createGameScreen() GtkWidget*
    }

    class ClientNetwork {
        -int socket_fd_
        -atomic~ConnectionStatus~ status_
        -uint32_t user_id_
        -string session_token_
        -string display_name_
        -int32_t elo_rating_
        -thread receive_thread_
        -atomic~bool~ running_
        -mutex callback_mutex_
        +ClientNetwork()
        +~ClientNetwork()
        +connect(host, port, callback) bool
        +disconnect() void
        +registerUser(username, password, display_name, callback) void
        +loginUser(username, password, callback) void
        +logoutUser(callback) void
        +isConnected() bool
        +isAuthenticated() bool
        -receiveLoop() void
        -handleAuthResponse(payload) void
    }

    class SessionStorage {
        <<utility>>
        +saveSession(user_id, token, username, display_name, elo) bool
        +loadSession(user_id&, token&, username&, display_name&, elo&) bool
        +clearSession() void
        +hasStoredSession() bool
        -getSessionFilePath() string
    }

    class AssetManager {
        -map~string,GdkPixbuf*~ textures
        -map~string,string~ sounds
        +AssetManager()
        +~AssetManager()
        +loadAssets() bool
        +getTexture(name) GdkPixbuf*
        +playSound(name) void
        -loadTexture(path) GdkPixbuf*
    }

    class ConnectionStatus {
        <<enumeration>>
        DISCONNECTED
        CONNECTING
        CONNECTED
        AUTHENTICATION_PENDING
        AUTHENTICATED
        ERROR_STATE
    }

    UIManager --> ClientNetwork : uses
    UIManager --> SessionStorage : uses
    UIManager --> AssetManager : uses
    ClientNetwork --> ConnectionStatus : has
```

**Description**:

### UIManager
- **Purpose**: Main GTK application controller
- **Responsibilities**:
  - Manage UI screens (login, lobby, game)
  - Handle user input events
  - Display game state
  - Coordinate with ClientNetwork for server communication
- **Thread Safety**: All GTK operations on main thread via `g_idle_add()`
- **File**: `client/src/ui_manager.cpp`

### ClientNetwork
- **Purpose**: Handle TCP socket communication with server
- **Responsibilities**:
  - Establish/maintain connection
  - Send authentication requests
  - Receive responses asynchronously
  - Invoke callbacks on main thread
- **Thread Safety**: Atomic status, mutex-protected callbacks
- **File**: `client/src/client_network.cpp`

### SessionStorage
- **Purpose**: Persist session data for auto-login
- **Storage**: `~/.battleship/session.txt` (0600 permissions)
- **Format**: Plain text, 5 lines (user_id, token, username, display_name, elo)
- **File**: `client/src/session_storage.cpp`

### AssetManager
- **Purpose**: Load and manage game assets
- **Assets**: Ship textures, water textures, sound effects
- **File**: `client/src/asset_manager.cpp`

---

## 2. Server-Side Components

```mermaid
classDiagram
    class Server {
        -int server_socket_
        -atomic~bool~ running_
        -DatabaseManager* database_
        -AuthHandler* auth_handler_
        -map~int,ClientConnection*~ clients_
        -mutex clients_mutex_
        -thread accept_thread_
        -int port_
        +Server(port)
        +~Server()
        +start() bool
        +stop() void
        +broadcast(message) void
        +getConnectedClients() int
        +getDatabase() DatabaseManager*
        -acceptClients() void
        -handleClient(socket) void
        -routeMessage(client, header, payload) void
        -removeClient(socket) void
    }

    class AuthHandler {
        -DatabaseManager* db_
        +AuthHandler(db)
        +handleRegister(request, response) void
        +handleLogin(request, response) void
        +handleLogout(request, response) void
        -validateUsername(username) bool
        -validatePassword(password) bool
        -generateSessionToken(user_id) string
    }

    class DatabaseManager {
        -sqlite3* db_
        -string db_path_
        -string last_error_
        +DatabaseManager(path)
        +~DatabaseManager()
        +isOpen() bool
        +createUser(username, password_hash, display_name) uint32_t
        +getUserByUsername(username) User
        +getUserById(user_id) User
        +updateLastLogin(user_id) bool
        +updateEloRating(user_id, elo) bool
        +usernameExists(username) bool
        +createSession(user_id, token, duration) uint32_t
        +validateSession(token) uint32_t
        +deleteSession(token) bool
        +createMatch(player1_id, player2_id) uint32_t
        +saveShipPlacement(match_id, user_id, ship_data) bool
        +saveMove(match_id, player_id, move_num, x, y, result) bool
        -initializeSchema() bool
    }

    class ClientConnection {
        -int socket_fd_
        -string client_address_
        -atomic~bool~ connected_
        -uint32_t user_id_
        -thread receive_thread_
        +ClientConnection(socket, address)
        +~ClientConnection()
        +send(header, payload) bool
        +receive(header, payload) bool
        +disconnect() void
        +isConnected() bool
        +setUserId(id) void
        +getUserId() uint32_t
    }

    Server --> AuthHandler : owns
    Server --> DatabaseManager : owns
    Server --> ClientConnection : manages
    AuthHandler --> DatabaseManager : uses
```

**Description**:

### Server
- **Purpose**: Main TCP server, manages client connections
- **Responsibilities**:
  - Accept incoming connections
  - Route messages to appropriate handlers
  - Manage client lifecycle
  - Broadcast messages
- **Thread Safety**: Mutex-protected clients map
- **File**: `server/src/server.cpp`

### AuthHandler
- **Purpose**: Process authentication requests
- **Responsibilities**:
  - Register new users (validate, hash password, create account)
  - Login users (verify credentials, create session)
  - Logout users (invalidate session)
  - Generate session tokens
- **Security**: Uses PasswordHash for secure password storage
- **File**: `server/src/auth_handler.cpp`

### DatabaseManager
- **Purpose**: SQLite database operations
- **Responsibilities**:
  - User CRUD operations
  - Session management
  - Match data persistence
  - Move history tracking
- **Configuration**: WAL mode, foreign keys enabled
- **File**: `server/src/database_manager.cpp`

### ClientConnection
- **Purpose**: Represents individual client connection
- **Responsibilities**:
  - Send/receive messages via socket
  - Track authentication state
  - Background receive thread
- **File**: `server/src/client_connection.cpp`

---

## 3. Common/Shared Components

```mermaid
classDiagram
    class Protocol {
        <<utility>>
        +parseHeader(buffer) MessageHeader
        +serializeHeader(header) vector~uint8_t~
        +validateMessage(header) bool
        +getMessageTypeName(type) string
    }

    class PasswordHash {
        <<utility>>
        +hashPassword(password) string
        +verifyPassword(password, stored_hash) bool
        -generateSalt() string
        -sha256(data) string
    }

    class MessageHeader {
        <<struct>>
        +uint8_t type
        +uint32_t length
        +uint64_t timestamp
        +char session_token[64]
    }

    class MessageType {
        <<enumeration>>
        AUTH_REGISTER
        AUTH_LOGIN
        AUTH_LOGOUT
        AUTH_RESPONSE
        PING
        PONG
        MATCHMAKING_REQUEST
        GAME_START
        SHIP_PLACEMENT
        MAKE_MOVE
        CHAT_MESSAGE
    }

    Protocol --> MessageHeader : parses
    Protocol --> MessageType : uses
```

**Description**:

### Protocol
- **Purpose**: Message serialization/deserialization utilities
- **MessageHeader Structure** (77 bytes total):
  - `type`: uint8_t (1 byte) - Message type ID
  - `length`: uint32_t (4 bytes) - Payload size
  - `timestamp`: uint64_t (8 bytes) - Unix timestamp
  - `session_token`: char[64] (64 bytes) - Authentication token
- **File**: `common/include/protocol.h`

### PasswordHash
- **Purpose**: Secure password hashing with SHA-256 + salt
- **Hash Format**: `<salt>$<hash>` (32 hex chars + $ + 64 hex chars)
- **Security**:
  - Random 16-byte salt per password
  - Constant-time comparison
  - OpenSSL RAND_bytes for cryptographic randomness
- **File**: `common/src/password_hash.cpp`

### MessageType
- **Purpose**: Identifies message type
- **Categories**:
  - Authentication (1-10): REGISTER, LOGIN, LOGOUT, RESPONSE
  - Matchmaking (20-29): REQUEST, ACCEPT, DECLINE
  - Game (30-59): START, SHIP_PLACEMENT, MOVE, RESULT
  - Chat (60-79): MESSAGE, PRIVATE_MESSAGE
  - System (100+): PING, PONG, ERROR

### MessageSerialization
- **Template Functions**:
  ```cpp
  template<typename T>
  std::string serialize(const T& data) {
      return std::string(reinterpret_cast<const char*>(&data), sizeof(T));
  }

  template<typename T>
  bool deserialize(const std::string& payload, T& data) {
      if (payload.size() != sizeof(T)) return false;
      memcpy(&data, payload.data(), sizeof(T));
      return true;
  }
  ```
- **Requirements**: Works with POD types using `__attribute__((packed))`

---

## 4. Game Logic Components

```mermaid
classDiagram
    class GameState {
        -Board player_board
        -Board opponent_board
        -vector~Ship~ ships
        -GameStatus status
        -uint32_t current_player_id
        -uint32_t opponent_id
        +GameState()
        +placeShip(ship, x, y, orientation) bool
        +makeMove(x, y) MoveResult
        +receiveAttack(x, y) AttackResult
        +isGameOver() bool
        +getWinner() uint32_t
        +allShipsPlaced() bool
        -isValidPlacement(ship, x, y, orientation) bool
    }

    class Board {
        -CellState grid[10][10]
        -vector~Ship~ ships
        +Board()
        +getCell(x, y) CellState
        +setCell(x, y, state) void
        +placeShip(ship, x, y, orientation) bool
        +receiveAttack(x, y) AttackResult
        +allShipsSunk() bool
        +serialize() string
        +deserialize(data) bool
        -isValidPosition(x, y) bool
    }

    class Ship {
        -ShipType type
        -int length
        -int x
        -int y
        -Orientation orientation
        -vector~bool~ hit_positions
        +Ship(type)
        +getType() ShipType
        +getLength() int
        +isSunk() bool
        +hit(position) void
        +occupiesPosition(x, y) bool
    }

    class CellState {
        <<enumeration>>
        EMPTY
        SHIP
        HIT
        MISS
    }

    class ShipType {
        <<enumeration>>
        CARRIER
        BATTLESHIP
        CRUISER
        SUBMARINE
        DESTROYER
    }

    class Orientation {
        <<enumeration>>
        HORIZONTAL
        VERTICAL
    }

    class GameStatus {
        <<enumeration>>
        WAITING
        PLACING_SHIPS
        IN_PROGRESS
        COMPLETED
        DRAW
    }

    class AttackResult {
        <<enumeration>>
        MISS
        HIT
        SUNK
        INVALID
    }

    GameState --> Board : contains 2
    GameState --> Ship : manages
    GameState --> GameStatus : has
    Board --> Ship : contains
    Board --> CellState : uses
    Board --> AttackResult : returns
    Ship --> ShipType : has
    Ship --> Orientation : has
```

**Description**:

### GameState
- **Purpose**: Core game logic and state management
- **Flow**:
  1. WAITING → PLACING_SHIPS (both players join)
  2. PLACING_SHIPS → IN_PROGRESS (all ships placed)
  3. IN_PROGRESS → COMPLETED (all ships sunk)
- **File**: `common/src/game_state.cpp`

### Board
- **Purpose**: 10x10 grid with ship placement and attack handling
- **Grid**: 2D array of CellState (EMPTY, SHIP, HIT, MISS)
- **Methods**:
  - `placeShip()`: Validate and place ship
  - `receiveAttack()`: Process attack, return result
  - `serialize()`/`deserialize()`: Save/load board state
- **File**: `common/src/board.cpp`

### Ship
- **Purpose**: Individual ship with hit tracking
- **Types and Lengths**:
  - CARRIER: 5 cells
  - BATTLESHIP: 4 cells
  - CRUISER: 3 cells
  - SUBMARINE: 3 cells
  - DESTROYER: 2 cells
- **Hit Tracking**: Boolean array for each position
- **File**: `common/src/ship.cpp`

### Enums Explained

**CellState**: Visual representation
- EMPTY: `[~]` (water)
- SHIP: `[S]` (ship)
- HIT: `[X]` (hit)
- MISS: `[O]` (miss)

**AttackResult**: Attack outcome
- MISS: Hit water
- HIT: Hit ship (not sunk)
- SUNK: Hit ship and sank it
- INVALID: Invalid coordinates or already attacked

**Orientation**: Ship direction
- HORIZONTAL: Extends right (x increases)
- VERTICAL: Extends down (y increases)

---

## 5. Authentication Messages

```mermaid
classDiagram
    class RegisterRequest {
        <<struct>>
        +char username[64]
        +char password[64]
        +char display_name[64]
    }

    class RegisterResponse {
        <<struct>>
        +bool success
        +uint32_t user_id
        +char error_message[256]
    }

    class LoginRequest {
        <<struct>>
        +char username[64]
        +char password[64]
    }

    class LoginResponse {
        <<struct>>
        +bool success
        +uint32_t user_id
        +char username[64]
        +char display_name[64]
        +int32_t elo_rating
        +char session_token[128]
        +char error_message[256]
    }

    class LogoutRequest {
        <<struct>>
        +char session_token[128]
    }

    class LogoutResponse {
        <<struct>>
        +bool success
    }

    RegisterRequest --> RegisterResponse : receives
    LoginRequest --> LoginResponse : receives
    LogoutRequest --> LogoutResponse : receives
```

**Description**:

### RegisterRequest (192 bytes)
- **username**: Desired username (4-20 chars, alphanumeric)
- **password**: Plain text password (8+ chars, hashed by server)
- **display_name**: Display name shown in UI

### RegisterResponse (261 bytes)
- **success**: Registration succeeded
- **user_id**: New user ID (0 if failed)
- **error_message**: Error message if failed

### LoginRequest (128 bytes)
- **username**: Username
- **password**: Password

### LoginResponse (521 bytes)
- **success**: Login succeeded
- **user_id**: User ID
- **username**: Username
- **display_name**: Display name
- **elo_rating**: Current ELO rating
- **session_token**: Session token for future requests
- **error_message**: Error message if failed

### LogoutRequest (128 bytes)
- **session_token**: Session token to invalidate

### LogoutResponse (1 byte)
- **success**: Logout succeeded

**All structs**: `__attribute__((packed))` for binary compatibility

**File**: `common/include/messages/authentication_messages.h`

---

## 6. Database Models

```mermaid
erDiagram
    USERS ||--o{ SESSIONS : "has many"
    USERS ||--o{ MATCHES : "player1"
    USERS ||--o{ MATCHES : "player2"
    USERS ||--o{ MATCHES : "winner"
    MATCHES ||--o{ MATCH_BOARDS : "has boards"
    MATCHES ||--o{ MATCH_MOVES : "has moves"
    USERS ||--o{ MATCH_BOARDS : "owns board"
    USERS ||--o{ MATCH_MOVES : "makes move"

    USERS {
        uint32_t user_id PK
        string username UK
        string password_hash
        string display_name
        int32_t elo_rating
        time_t created_at
        time_t last_login
    }

    SESSIONS {
        uint32_t session_id PK
        uint32_t user_id FK
        string session_token UK
        time_t created_at
        time_t expires_at
    }

    MATCHES {
        uint32_t match_id PK
        uint32_t player1_id FK
        uint32_t player2_id FK
        uint32_t winner_id FK
        string status
        time_t created_at
        time_t ended_at
    }

    MATCH_BOARDS {
        uint32_t board_id PK
        uint32_t match_id FK
        uint32_t user_id FK
        string ship_positions
        time_t created_at
    }

    MATCH_MOVES {
        uint32_t move_id PK
        uint32_t match_id FK
        uint32_t player_id FK
        int32_t move_number
        int8_t x
        int8_t y
        string result
        time_t created_at
    }
```

**Description**:

### User Model
```cpp
struct User {
    uint32_t user_id;
    string username;
    string password_hash;
    string display_name;
    int32_t elo_rating;
    time_t created_at;
    time_t last_login;
};
```
- **File**: `common/include/models/user.h`

### Session Model
```cpp
struct Session {
    uint32_t session_id;
    uint32_t user_id;
    string session_token;
    time_t created_at;
    time_t expires_at;

    bool isExpired() {
        return time(nullptr) > expires_at;
    }
};
```
- **File**: `common/include/models/session.h`

### Match Model
```cpp
struct Match {
    uint32_t match_id;
    uint32_t player1_id;
    uint32_t player2_id;
    uint32_t winner_id;
    string status;
    time_t created_at;
    time_t ended_at;
};
```
- **File**: `common/include/models/match.h`

### Player Model (UI Display)
```cpp
struct Player {
    uint32_t user_id;
    string username;
    string display_name;
    int32_t elo_rating;
    PlayerStatus status;  // ONLINE, OFFLINE, IN_GAME, AWAY
};
```
- **File**: `common/include/models/player.h`

---

## 7. Message Flow Diagrams

### 7.1 Registration Flow

```mermaid
sequenceDiagram
    participant UI as UIManager
    participant CN as ClientNetwork
    participant S as Server
    participant AH as AuthHandler
    participant DB as DatabaseManager
    participant PH as PasswordHash

    UI->>CN: registerUser(username, password, display_name)
    CN->>CN: Create RegisterRequest
    CN->>S: Send AUTH_REGISTER message
    S->>AH: handleRegister(request)
    AH->>AH: validateUsername()
    AH->>AH: validatePassword()
    AH->>DB: usernameExists(username)
    DB-->>AH: false
    AH->>PH: hashPassword(password)
    PH-->>AH: password_hash
    AH->>DB: createUser(username, hash, display_name)
    DB-->>AH: user_id
    AH->>AH: Create RegisterResponse
    AH-->>S: response
    S->>CN: Send AUTH_RESPONSE message
    CN->>CN: handleAuthResponse()
    CN->>UI: invoke register_callback(success, user_id)
    UI->>UI: Show success message
```

### 7.2 Login Flow

```mermaid
sequenceDiagram
    participant UI as UIManager
    participant CN as ClientNetwork
    participant SS as SessionStorage
    participant S as Server
    participant AH as AuthHandler
    participant DB as DatabaseManager
    participant PH as PasswordHash

    UI->>CN: loginUser(username, password)
    CN->>CN: Create LoginRequest
    CN->>S: Send AUTH_LOGIN message
    S->>AH: handleLogin(request)
    AH->>DB: getUserByUsername(username)
    DB-->>AH: user
    AH->>PH: verifyPassword(password, user.password_hash)
    PH-->>AH: true
    AH->>AH: generateSessionToken(user_id)
    AH->>DB: createSession(user_id, token)
    DB-->>AH: session_id
    AH->>DB: updateLastLogin(user_id)
    AH->>AH: Create LoginResponse
    AH-->>S: response
    S->>CN: Send AUTH_RESPONSE message
    CN->>CN: handleAuthResponse()
    CN->>CN: Update user_id, session_token, status
    CN->>UI: invoke login_callback(success, user_data)
    UI->>SS: saveSession(user_id, token, ...)
    UI->>UI: showScreen(MAIN_MENU)
```

### 7.3 Auto-Login Flow

```mermaid
sequenceDiagram
    participant UI as UIManager
    participant SS as SessionStorage
    participant CN as ClientNetwork
    participant S as Server
    participant DB as DatabaseManager

    UI->>SS: hasStoredSession()
    SS-->>UI: true
    UI->>SS: loadSession()
    SS-->>UI: user_id, token, username, display_name, elo
    UI->>CN: connect(host, port)
    CN->>S: TCP connection
    S-->>CN: Connected
    UI->>CN: validateSession(token)
    CN->>S: Send VALIDATE_SESSION message
    S->>DB: validateSession(token)
    DB-->>S: user_id (or 0 if invalid)
    alt Session Valid
        S->>CN: Send SUCCESS response
        CN->>UI: invoke callback(true)
        UI->>UI: Restore user state
        UI->>UI: showScreen(MAIN_MENU)
    else Session Invalid/Expired
        S->>CN: Send FAILURE response
        CN->>UI: invoke callback(false)
        UI->>SS: clearSession()
        UI->>UI: showScreen(LOGIN)
    end
```

### 7.4 Game Flow

```mermaid
sequenceDiagram
    participant P1 as Player 1
    participant S as Server
    participant P2 as Player 2
    participant DB as DatabaseManager

    P1->>S: MATCHMAKING_REQUEST
    P2->>S: MATCHMAKING_REQUEST
    S->>DB: createMatch(player1_id, player2_id)
    S->>P1: GAME_START (opponent info)
    S->>P2: GAME_START (opponent info)

    Note over P1,P2: Ship Placement Phase
    P1->>S: SHIP_PLACEMENT (board data)
    S->>DB: saveShipPlacement(match_id, player1_id, data)
    P2->>S: SHIP_PLACEMENT (board data)
    S->>DB: saveShipPlacement(match_id, player2_id, data)

    Note over P1,P2: Game Active Phase
    loop Until Game Over
        P1->>S: MAKE_MOVE (x, y)
        S->>S: Process attack on P2's board
        S->>DB: saveMove(match_id, player1_id, x, y, result)
        S->>P1: MOVE_RESULT (HIT/MISS/SUNK)
        S->>P2: OPPONENT_MOVE (x, y, result)

        alt Ship Sunk
            S->>P1: SHIP_SUNK (ship_type)
            S->>P2: YOUR_SHIP_SUNK (ship_type)
        end

        alt All Ships Sunk
            S->>DB: endMatch(match_id, winner_id)
            S->>P1: GAME_OVER (YOU_WIN)
            S->>P2: GAME_OVER (YOU_LOSE)
        else Game Continues
            P2->>S: MAKE_MOVE (x, y)
            Note over P1,P2: Player 2's turn...
        end
    end
```

### 7.5 Thread Communication

```mermaid
flowchart TB
    subgraph "Client Process"
        GT[GTK Main Thread]
        RT[Receive Thread]

        GT -->|1. User clicks login| GT
        GT -->|2. Call loginUser| CN[ClientNetwork]
        CN -->|3. Send message| Socket

        Socket -->|4. Receive response| RT
        RT -->|5. Parse message| RT
        RT -->|6. g_idle_add| Queue[GTK Event Queue]
        Queue -->|7. Execute on main thread| GT
        GT -->|8. Update UI| GT
    end

    subgraph "Server Process"
        Socket <-->|TCP| ServerSocket[Server Socket]
    end

    style GT fill:#90EE90
    style RT fill:#FFB6C1
    style Queue fill:#FFD700
```

**Key Points**:
- **GTK Main Thread**: Handles all UI operations
- **Receive Thread**: Background thread for network I/O
- **g_idle_add**: Safely dispatch callbacks to main thread
- **Mutex**: Protects shared callback data

---

## 8. Design Patterns Summary

```mermaid
graph TB
    subgraph "Singleton Pattern"
        S[Server]
        DM[DatabaseManager]
    end

    subgraph "Observer Pattern"
        CN[ClientNetwork<br/>Callbacks]
        GTK[GTK Event<br/>Handlers]
    end

    subgraph "Factory Pattern"
        MF[Message<br/>createHeader]
        SF[Screen<br/>createLoginScreen]
    end

    subgraph "Repository Pattern"
        DB[DatabaseManager<br/>Data Access Layer]
    end

    subgraph "Strategy Pattern"
        AH[AuthHandler]
        GH[GameHandler]
        CH[ChatHandler]
    end
```

**Descriptions**:

- **Singleton**: Server and DatabaseManager have single instances
- **Observer**: Callbacks notify when async events occur
- **Factory**: Factory methods create complex objects (messages, screens)
- **Repository**: DatabaseManager abstracts SQL, provides clean API
- **Strategy**: Different handlers for different message types

---

## 9. Security Considerations

```mermaid
graph LR
    A[Password] -->|User Input| B[PasswordHash::hashPassword]
    B -->|Random Salt| C[SHA-256]
    C -->|salt$hash| D[Database]

    E[Login Attempt] -->|User Input| F[PasswordHash::verifyPassword]
    F -->|Extract Salt| G[SHA-256]
    D -->|Stored Hash| H[Constant-Time Compare]
    G --> H
    H -->|Match| I[Success]
    H -->|No Match| J[Failure]

    style B fill:#90EE90
    style F fill:#90EE90
    style H fill:#FFB6C1
```

**Security Features**:

1. **Password Hashing**:
   - SHA-256 with random 16-byte salt
   - Never store plain text passwords
   - Each password gets unique salt

2. **Constant-Time Comparison**:
   - Prevents timing attacks
   - Compare all characters even if mismatch found early

3. **Session Tokens**:
   - 128-character random tokens
   - Expire after 24 hours
   - Stored securely in database

4. **File Permissions**:
   - Session file: 0600 (owner only)
   - Database file: 0600 (owner only)

5. **SQL Injection Prevention**:
   - Prepared statements
   - Parameter binding
   - No string concatenation

6. **Input Validation**:
   - Username: 4-20 chars, alphanumeric
   - Password: 8+ chars
   - Display name: 64 chars max

---

Last Updated: 2025-11-21
