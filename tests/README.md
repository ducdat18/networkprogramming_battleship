# Battleship Testing Suite

## Cấu Trúc Thư Mục

```
tests/
├── unit_tests/              # Unit tests - test từng module riêng lẻ
│   ├── board/               # Tests cho Board class
│   │   └── test_board.cpp   # Ship placement, shot processing, victory
│   ├── match/               # Tests cho MatchState class
│   │   └── test_match.cpp   # Turn management, move processing, game flow
│   ├── protocol/            # Tests cho message protocol
│   │   └── test_protocol.cpp (TODO)
│   └── network/             # Tests cho networking code
│       └── test_network.cpp (TODO)
│
├── integration_tests/       # Integration tests - test nhiều modules kết hợp
│   ├── test_client_server.cpp (TODO)
│   └── test_game_flow.cpp (TODO)
│
└── README.md               # File này
```

## Unit Tests vs Integration Tests

### Unit Tests
- **Mục đích**: Test từng function/class riêng lẻ
- **Đặc điểm**:
  - Nhanh (< 1s mỗi test)
  - Không cần network/database
  - Không có dependencies phức tạp
  - Dễ debug khi fail
- **Ví dụ**: Test Board::placeShip() có validate đúng không

### Integration Tests
- **Mục đích**: Test nhiều components làm việc cùng nhau
- **Đặc điểm**:
  - Chậm hơn (có thể 5-10s)
  - Cần setup môi trường (mock server, sockets)
  - Test real-world scenarios
- **Ví dụ**: Test client kết nối server → login → play game → logout

## Chạy Tests

```bash
# Build và chạy tất cả tests
make test

# Chỉ build tests (không chạy)
make tests

# Chạy từng loại tests
make test-board          # Unit tests: Board logic
make test-match          # Unit tests: Match logic
make test-unit           # Tất cả unit tests
make test-integration    # Tất cả integration tests

# Clean tests
make clean-tests
```

## Viết Test Mới

### 1. Chọn Loại Test
- **Unit test?** → Đặt trong `unit_tests/<module>/`
- **Integration test?** → Đặt trong `integration_tests/`

### 2. Tạo File Test
```cpp
#include <gtest/gtest.h>
#include "../../common/include/<header>.h"

class MyTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup trước mỗi test
    }

    void TearDown() override {
        // Cleanup sau mỗi test
    }
};

TEST_F(MyTest, TestName) {
    EXPECT_EQ(actual, expected);
}
```

### 3. Update Makefile
Thêm test mới vào `Makefile`:
```makefile
TEST_MYNEW = $(BIN_DIR)/test_mynew
$(TEST_MYNEW): tests/unit_tests/mynew/test_mynew.cpp $(COMMON_OBJECTS)
    $(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@ $(GTEST_FLAGS)
```

## Google Test Assertions

### Equality
```cpp
EXPECT_EQ(a, b)      // a == b
EXPECT_NE(a, b)      // a != b
EXPECT_LT(a, b)      // a < b
EXPECT_LE(a, b)      // a <= b
EXPECT_GT(a, b)      // a > b
EXPECT_GE(a, b)      // a >= b
```

### Boolean
```cpp
EXPECT_TRUE(condition)
EXPECT_FALSE(condition)
```

### Floating Point
```cpp
EXPECT_FLOAT_EQ(a, b)
EXPECT_DOUBLE_EQ(a, b)
```

### String
```cpp
EXPECT_STREQ(str1, str2)    // C-strings
EXPECT_STRNE(str1, str2)
```

### EXPECT vs ASSERT
- `EXPECT_*`: Test tiếp tục nếu fail
- `ASSERT_*`: Stop ngay khi fail

## Testing Roadmap

### Phase 1: Core Logic (DONE)
- [x] Board placement logic
- [x] Board shot processing
- [x] Match turn management
- [x] Match move processing

### Phase 2: Protocol & Serialization (TODO)
- [ ] Message serialization/deserialization
- [ ] Protocol validation
- [ ] Edge cases (invalid messages, buffer overflow)

### Phase 3: Network Layer (TODO)
- [ ] Socket creation/binding
- [ ] Connection handling
- [ ] Message sending/receiving
- [ ] Error handling (disconnect, timeout)

### Phase 4: Integration (TODO)
- [ ] Client-server handshake
- [ ] Full game flow (login → play → logout)
- [ ] Multiple clients
- [ ] Stress testing

## Best Practices

1. **Test tên rõ ràng**: `TEST(BoardTest, PlaceShip_OutOfBounds_ReturnsFalse)`
2. **Một test một điều**: Mỗi test chỉ verify 1 behavior
3. **AAA pattern**: Arrange → Act → Assert
4. **Độc lập**: Tests không depend vào nhau
5. **Fast**: Unit tests phải < 1s
6. **Deterministic**: Cùng input → cùng output (tránh random)

## Debugging Failed Tests

```bash
# Chạy test với verbose output
./bin/test_board --gtest_verbose

# Chạy chỉ 1 test cụ thể
./bin/test_board --gtest_filter=BoardTest.PlaceShip_Valid

# Chạy test với valgrind (check memory leaks)
valgrind --leak-check=full ./bin/test_board
```

## CI/CD Integration (Future)

```yaml
# .github/workflows/test.yml
name: Run Tests
on: [push, pull_request]
jobs:
  test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      - run: make install-deps
      - run: make test
```
