/**
 * Unit tests for authentication message serialization
 * Tests serialize/deserialize for Login, Register, Logout messages
 */

#include <gtest/gtest.h>
#include "messages/authentication_messages.h"
#include "message_serialization.h"
#include <cstring>

using namespace MessageSerialization;

// ==================== RegisterRequest Tests ====================

TEST(AuthMessages, RegisterRequest_SerializeDeserialize) {
    // Create original request
    RegisterRequest original;
    safeStrCopy(original.username, "testuser", sizeof(original.username));
    safeStrCopy(original.password, "hashed_password_12345", sizeof(original.password));
    safeStrCopy(original.display_name, "Test User", sizeof(original.display_name));

    // Serialize
    std::string serialized = serialize(original);

    // Verify size
    EXPECT_EQ(serialized.size(), sizeof(RegisterRequest));

    // Deserialize
    RegisterRequest deserialized;
    ASSERT_TRUE(deserialize(serialized, deserialized));

    // Verify fields
    EXPECT_STREQ(deserialized.username, "testuser");
    EXPECT_STREQ(deserialized.password, "hashed_password_12345");
    EXPECT_STREQ(deserialized.display_name, "Test User");
}

TEST(AuthMessages, RegisterRequest_MaxLengthStrings) {
    RegisterRequest original;

    // Fill with maximum length strings
    std::string max_username(31, 'a');  // 31 chars + null terminator = 32
    std::string max_password(63, 'b');  // 63 chars + null terminator = 64
    std::string max_display(63, 'c');   // 63 chars + null terminator = 64

    safeStrCopy(original.username, max_username, sizeof(original.username));
    safeStrCopy(original.password, max_password, sizeof(original.password));
    safeStrCopy(original.display_name, max_display, sizeof(original.display_name));

    // Serialize and deserialize
    std::string serialized = serialize(original);
    RegisterRequest deserialized;
    ASSERT_TRUE(deserialize(serialized, deserialized));

    // Verify
    EXPECT_STREQ(deserialized.username, max_username.c_str());
    EXPECT_STREQ(deserialized.password, max_password.c_str());
    EXPECT_STREQ(deserialized.display_name, max_display.c_str());
}

// ==================== RegisterResponse Tests ====================

TEST(AuthMessages, RegisterResponse_Success) {
    RegisterResponse original;
    original.success = true;
    original.user_id = 42;
    memset(original.error_message, 0, sizeof(original.error_message));

    // Serialize and deserialize
    std::string serialized = serialize(original);
    RegisterResponse deserialized;
    ASSERT_TRUE(deserialize(serialized, deserialized));

    // Verify
    EXPECT_TRUE(deserialized.success);
    EXPECT_EQ(deserialized.user_id, 42);
    EXPECT_STREQ(deserialized.error_message, "");
}

TEST(AuthMessages, RegisterResponse_Failure) {
    RegisterResponse original;
    original.success = false;
    original.user_id = 0;
    safeStrCopy(original.error_message, "Username already exists", sizeof(original.error_message));

    // Serialize and deserialize
    std::string serialized = serialize(original);
    RegisterResponse deserialized;
    ASSERT_TRUE(deserialize(serialized, deserialized));

    // Verify
    EXPECT_FALSE(deserialized.success);
    EXPECT_EQ(deserialized.user_id, 0);
    EXPECT_STREQ(deserialized.error_message, "Username already exists");
}

// ==================== LoginRequest Tests ====================

TEST(AuthMessages, LoginRequest_SerializeDeserialize) {
    LoginRequest original;
    safeStrCopy(original.username, "johndoe", sizeof(original.username));
    safeStrCopy(original.password, "sha256_hash_value", sizeof(original.password));

    // Serialize and deserialize
    std::string serialized = serialize(original);
    LoginRequest deserialized;
    ASSERT_TRUE(deserialize(serialized, deserialized));

    // Verify
    EXPECT_STREQ(deserialized.username, "johndoe");
    EXPECT_STREQ(deserialized.password, "sha256_hash_value");
}

TEST(AuthMessages, LoginRequest_EmptyPassword) {
    LoginRequest original;
    safeStrCopy(original.username, "testuser", sizeof(original.username));
    memset(original.password, 0, sizeof(original.password));

    // Serialize and deserialize
    std::string serialized = serialize(original);
    LoginRequest deserialized;
    ASSERT_TRUE(deserialize(serialized, deserialized));

    // Verify
    EXPECT_STREQ(deserialized.username, "testuser");
    EXPECT_STREQ(deserialized.password, "");
}

// ==================== LoginResponse Tests ====================

TEST(AuthMessages, LoginResponse_Success) {
    LoginResponse original;
    original.success = true;
    original.user_id = 123;
    safeStrCopy(original.session_token, "token_123_abc123def456", sizeof(original.session_token));
    safeStrCopy(original.display_name, "John Doe", sizeof(original.display_name));
    original.elo_rating = 1500;
    memset(original.error_message, 0, sizeof(original.error_message));

    // Serialize and deserialize
    std::string serialized = serialize(original);
    LoginResponse deserialized;
    ASSERT_TRUE(deserialize(serialized, deserialized));

    // Verify
    EXPECT_TRUE(deserialized.success);
    EXPECT_EQ(deserialized.user_id, 123);
    EXPECT_STREQ(deserialized.session_token, "token_123_abc123def456");
    EXPECT_STREQ(deserialized.display_name, "John Doe");
    EXPECT_EQ(deserialized.elo_rating, 1500);
    EXPECT_STREQ(deserialized.error_message, "");
}

TEST(AuthMessages, LoginResponse_Failure) {
    LoginResponse original;
    original.success = false;
    original.user_id = 0;
    memset(original.session_token, 0, sizeof(original.session_token));
    memset(original.display_name, 0, sizeof(original.display_name));
    original.elo_rating = 0;
    safeStrCopy(original.error_message, "Invalid password", sizeof(original.error_message));

    // Serialize and deserialize
    std::string serialized = serialize(original);
    LoginResponse deserialized;
    ASSERT_TRUE(deserialize(serialized, deserialized));

    // Verify
    EXPECT_FALSE(deserialized.success);
    EXPECT_EQ(deserialized.user_id, 0);
    EXPECT_STREQ(deserialized.session_token, "");
    EXPECT_STREQ(deserialized.display_name, "");
    EXPECT_EQ(deserialized.elo_rating, 0);
    EXPECT_STREQ(deserialized.error_message, "Invalid password");
}

// ==================== LogoutRequest Tests ====================

TEST(AuthMessages, LogoutRequest_SerializeDeserialize) {
    LogoutRequest original;
    safeStrCopy(original.session_token, "token_456_xyz789ghi012", sizeof(original.session_token));

    // Serialize and deserialize
    std::string serialized = serialize(original);
    LogoutRequest deserialized;
    ASSERT_TRUE(deserialize(serialized, deserialized));

    // Verify
    EXPECT_STREQ(deserialized.session_token, "token_456_xyz789ghi012");
}

// ==================== LogoutResponse Tests ====================

TEST(AuthMessages, LogoutResponse_Success) {
    LogoutResponse original;
    original.success = true;

    // Serialize and deserialize
    std::string serialized = serialize(original);
    LogoutResponse deserialized;
    ASSERT_TRUE(deserialize(serialized, deserialized));

    // Verify
    EXPECT_TRUE(deserialized.success);
}

TEST(AuthMessages, LogoutResponse_Failure) {
    LogoutResponse original;
    original.success = false;

    // Serialize and deserialize
    std::string serialized = serialize(original);
    LogoutResponse deserialized;
    ASSERT_TRUE(deserialize(serialized, deserialized));

    // Verify
    EXPECT_FALSE(deserialized.success);
}

// ==================== Size Validation Tests ====================

TEST(AuthMessages, InvalidSize_DeserializeFails) {
    // Create a payload with incorrect size
    std::string invalid_payload = "too_short";

    RegisterRequest reg;
    EXPECT_FALSE(deserialize(invalid_payload, reg));

    LoginRequest login;
    EXPECT_FALSE(deserialize(invalid_payload, login));

    LogoutRequest logout;
    EXPECT_FALSE(deserialize(invalid_payload, logout));
}

TEST(AuthMessages, StructSizes_AreCorrect) {
    // Verify struct sizes match actual platform sizes
    // Note: Even with __attribute__((packed)), sizes may vary due to alignment

    // These are the actual sizes on this platform
    EXPECT_EQ(sizeof(RegisterRequest), 288);
    EXPECT_EQ(sizeof(RegisterResponse), 133);
    EXPECT_EQ(sizeof(LoginRequest), 96);
    EXPECT_EQ(sizeof(LoginResponse), 265);
    EXPECT_EQ(sizeof(LogoutRequest), 64);
    EXPECT_EQ(sizeof(LogoutResponse), 1);
}

// ==================== Main ====================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
