#ifndef SESSION_STORAGE_H
#define SESSION_STORAGE_H

#include <string>
#include <cstdint>

/**
 * Session storage for client-side session persistence
 * Stores session token in ~/.battleship/session.txt
 */
class SessionStorage {
public:
    /**
     * Save session data to file
     * @param user_id User ID
     * @param session_token Session token
     * @param username Username
     * @param display_name Display name
     * @param elo_rating ELO rating
     * @return true if saved successfully
     */
    static bool saveSession(uint32_t user_id,
                           const std::string& session_token,
                           const std::string& username,
                           const std::string& display_name,
                           int32_t elo_rating);

    /**
     * Load session data from file
     * @param user_id Output: User ID
     * @param session_token Output: Session token
     * @param username Output: Username
     * @param display_name Output: Display name
     * @param elo_rating Output: ELO rating
     * @return true if loaded successfully
     */
    static bool loadSession(uint32_t& user_id,
                           std::string& session_token,
                           std::string& username,
                           std::string& display_name,
                           int32_t& elo_rating);

    /**
     * Clear stored session (logout)
     */
    static void clearSession();

    /**
     * Check if session file exists
     */
    static bool hasStoredSession();

private:
    /**
     * Get session file path (~/.battleship/session.txt)
     */
    static std::string getSessionFilePath();
};

#endif // SESSION_STORAGE_H
