#include "password_hash.h"
#include <openssl/sha.h>
#include <openssl/rand.h>
#include <sstream>
#include <iomanip>
#include <cstring>

std::string PasswordHash::hashPassword(const std::string& password) {
    // Generate random salt
    std::string salt = generateSalt();

    // Hash the salt + password
    std::string hash = sha256(salt + password);

    // Return in format: salt$hash
    return salt + "$" + hash;
}

bool PasswordHash::verifyPassword(const std::string& password, const std::string& stored_hash) {
    // Parse stored hash to extract salt
    size_t dollar_pos = stored_hash.find('$');
    if (dollar_pos == std::string::npos || dollar_pos == 0) {
        return false;  // Invalid format
    }

    std::string salt = stored_hash.substr(0, dollar_pos);
    std::string expected_hash = stored_hash.substr(dollar_pos + 1);

    // Compute hash of salt + password
    std::string computed_hash = sha256(salt + password);

    // Constant-time comparison to prevent timing attacks
    if (computed_hash.length() != expected_hash.length()) {
        return false;
    }

    unsigned char result = 0;
    for (size_t i = 0; i < computed_hash.length(); i++) {
        result |= (computed_hash[i] ^ expected_hash[i]);
    }

    return result == 0;
}

std::string PasswordHash::generateSalt() {
    // Generate 16 random bytes
    unsigned char salt_bytes[16];

    if (RAND_bytes(salt_bytes, sizeof(salt_bytes)) != 1) {
        // Fallback to simple random if OpenSSL RAND fails
        for (int i = 0; i < 16; i++) {
            salt_bytes[i] = static_cast<unsigned char>(rand() % 256);
        }
    }

    // Convert to hex string
    std::stringstream ss;
    for (int i = 0; i < 16; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0')
           << static_cast<int>(salt_bytes[i]);
    }

    return ss.str();
}

std::string PasswordHash::sha256(const std::string& data) {
    // Compute SHA-256 hash
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(data.c_str()),
           data.length(), hash);

    // Convert to hex string
    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0')
           << static_cast<int>(hash[i]);
    }

    return ss.str();
}
