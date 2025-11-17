#ifndef PASSWORD_HASH_H
#define PASSWORD_HASH_H

#include <string>

/**
 * Password hashing utilities using SHA-256 with salt
 *
 * Format: salt$hash
 * - salt: 16-byte random hex string (32 chars)
 * - hash: SHA-256 hash of (salt + password) in hex (64 chars)
 * - Total stored: ~97 chars
 */
class PasswordHash {
public:
    /**
     * Hash a password with random salt
     * @param password Plain text password
     * @return Hashed password in format "salt$hash"
     */
    static std::string hashPassword(const std::string& password);

    /**
     * Verify a password against a stored hash
     * @param password Plain text password to verify
     * @param stored_hash Stored hash in format "salt$hash"
     * @return true if password matches, false otherwise
     */
    static bool verifyPassword(const std::string& password, const std::string& stored_hash);

private:
    /**
     * Generate random salt (16 bytes = 32 hex chars)
     */
    static std::string generateSalt();

    /**
     * Compute SHA-256 hash
     * @param data Data to hash
     * @return Hex string of hash (64 chars)
     */
    static std::string sha256(const std::string& data);
};

#endif // PASSWORD_HASH_H
