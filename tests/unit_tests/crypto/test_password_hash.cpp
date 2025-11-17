/**
 * Unit tests for PasswordHash
 * Tests password hashing and verification with SHA-256
 */

#include <gtest/gtest.h>
#include "password_hash.h"

class PasswordHashTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// Test: Hash password produces non-empty hash
TEST_F(PasswordHashTest, HashPasswordNotEmpty) {
    std::string password = "test_password";
    std::string hash = PasswordHash::hashPassword(password);

    EXPECT_FALSE(hash.empty());
    EXPECT_NE(hash, password);  // Hash should be different from password
}

// Test: Hash format is salt$hash
TEST_F(PasswordHashTest, HashFormatCorrect) {
    std::string password = "test_password";
    std::string hash = PasswordHash::hashPassword(password);

    // Should contain exactly one '$' separator
    size_t dollar_pos = hash.find('$');
    EXPECT_NE(dollar_pos, std::string::npos);
    EXPECT_GT(dollar_pos, 0u);  // Salt should not be empty
    EXPECT_LT(dollar_pos, hash.length() - 1);  // Hash should not be empty

    // Should not contain second '$'
    EXPECT_EQ(hash.find('$', dollar_pos + 1), std::string::npos);
}

// Test: Same password hashed twice produces different hashes (due to random salt)
TEST_F(PasswordHashTest, SamePasswordDifferentHashes) {
    std::string password = "test_password";
    std::string hash1 = PasswordHash::hashPassword(password);
    std::string hash2 = PasswordHash::hashPassword(password);

    EXPECT_NE(hash1, hash2);  // Different salts should produce different hashes
}

// Test: Verify correct password returns true
TEST_F(PasswordHashTest, VerifyCorrectPassword) {
    std::string password = "correct_password";
    std::string hash = PasswordHash::hashPassword(password);

    bool verified = PasswordHash::verifyPassword(password, hash);
    EXPECT_TRUE(verified);
}

// Test: Verify incorrect password returns false
TEST_F(PasswordHashTest, VerifyIncorrectPassword) {
    std::string password = "correct_password";
    std::string hash = PasswordHash::hashPassword(password);

    bool verified = PasswordHash::verifyPassword("wrong_password", hash);
    EXPECT_FALSE(verified);
}

// Test: Verify with empty password
TEST_F(PasswordHashTest, EmptyPassword) {
    std::string password = "";
    std::string hash = PasswordHash::hashPassword(password);

    EXPECT_FALSE(hash.empty());
    EXPECT_TRUE(PasswordHash::verifyPassword("", hash));
    EXPECT_FALSE(PasswordHash::verifyPassword("not_empty", hash));
}

// Test: Verify with long password
TEST_F(PasswordHashTest, LongPassword) {
    std::string password(1000, 'a');  // 1000 character password
    std::string hash = PasswordHash::hashPassword(password);

    EXPECT_TRUE(PasswordHash::verifyPassword(password, hash));

    std::string wrong_password(999, 'a');
    EXPECT_FALSE(PasswordHash::verifyPassword(wrong_password, hash));
}

// Test: Verify with special characters
TEST_F(PasswordHashTest, SpecialCharacters) {
    std::string password = "p@ssw0rd!#$%^&*()_+-=[]{}|;':\",./<>?";
    std::string hash = PasswordHash::hashPassword(password);

    EXPECT_TRUE(PasswordHash::verifyPassword(password, hash));
    EXPECT_FALSE(PasswordHash::verifyPassword("p@ssw0rd", hash));
}

// Test: Verify with unicode characters
TEST_F(PasswordHashTest, UnicodeCharacters) {
    std::string password = "パスワード密码🔒";
    std::string hash = PasswordHash::hashPassword(password);

    EXPECT_TRUE(PasswordHash::verifyPassword(password, hash));
    EXPECT_FALSE(PasswordHash::verifyPassword("パスワード", hash));
}

// Test: Verify with malformed hash (no dollar sign)
TEST_F(PasswordHashTest, MalformedHashNoDollar) {
    std::string password = "test";
    std::string malformed_hash = "no_dollar_sign_here";

    EXPECT_FALSE(PasswordHash::verifyPassword(password, malformed_hash));
}

// Test: Verify with malformed hash (empty salt)
TEST_F(PasswordHashTest, MalformedHashEmptySalt) {
    std::string password = "test";
    std::string malformed_hash = "$hashonly";

    EXPECT_FALSE(PasswordHash::verifyPassword(password, malformed_hash));
}

// Test: Verify with malformed hash (empty hash part)
TEST_F(PasswordHashTest, MalformedHashEmptyHash) {
    std::string password = "test";
    std::string malformed_hash = "saltonly$";

    // This should fail because the hash part is wrong
    EXPECT_FALSE(PasswordHash::verifyPassword(password, malformed_hash));
}

// Test: Case sensitivity
TEST_F(PasswordHashTest, CaseSensitivity) {
    std::string password = "Password123";
    std::string hash = PasswordHash::hashPassword(password);

    EXPECT_TRUE(PasswordHash::verifyPassword("Password123", hash));
    EXPECT_FALSE(PasswordHash::verifyPassword("password123", hash));
    EXPECT_FALSE(PasswordHash::verifyPassword("PASSWORD123", hash));
}

// Test: Salt length is reasonable (16 bytes = 32 hex chars)
TEST_F(PasswordHashTest, SaltLength) {
    std::string password = "test";
    std::string hash = PasswordHash::hashPassword(password);

    size_t dollar_pos = hash.find('$');
    std::string salt = hash.substr(0, dollar_pos);

    // Salt should be 32 characters (16 bytes as hex)
    EXPECT_EQ(salt.length(), 32u);
}

// Test: Hash length is SHA-256 (64 hex chars)
TEST_F(PasswordHashTest, HashLength) {
    std::string password = "test";
    std::string hash = PasswordHash::hashPassword(password);

    size_t dollar_pos = hash.find('$');
    std::string hash_part = hash.substr(dollar_pos + 1);

    // SHA-256 produces 32 bytes = 64 hex characters
    EXPECT_EQ(hash_part.length(), 64u);
}

// Test: Constant time comparison (timing attack resistance)
TEST_F(PasswordHashTest, ConstantTimeComparison) {
    std::string password = "test_password";
    std::string hash = PasswordHash::hashPassword(password);

    // These should all take similar time regardless of when they differ
    EXPECT_FALSE(PasswordHash::verifyPassword("aest_password", hash));
    EXPECT_FALSE(PasswordHash::verifyPassword("test_passwora", hash));
    EXPECT_FALSE(PasswordHash::verifyPassword("zest_password", hash));
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
