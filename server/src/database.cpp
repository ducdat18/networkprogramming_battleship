#include "database.h"
#include <iostream>
#include <sstream>
#include <cstring>
#include <sys/stat.h>
#include <sys/types.h>

DatabaseManager::DatabaseManager(const std::string& db_path)
    : db_(nullptr), db_path_(db_path), last_error_("") {

    // Create data directory if it doesn't exist
    std::string dir = db_path.substr(0, db_path.find_last_of('/'));
    if (!dir.empty()) {
        mkdir(dir.c_str(), 0755);
    }

    // Open database
    int rc = sqlite3_open(db_path.c_str(), &db_);
    if (rc != SQLITE_OK) {
        last_error_ = "Failed to open database: " + std::string(sqlite3_errmsg(db_));
        std::cerr << "[DB] " << last_error_ << std::endl;
        sqlite3_close(db_);
        db_ = nullptr;
        return;
    }

    std::cout << "[DB] Database opened: " << db_path << std::endl;

    // Enable WAL mode for better concurrent access
    executeSQL("PRAGMA journal_mode=WAL;");

    // Enable foreign keys
    executeSQL("PRAGMA foreign_keys = ON;");

    // Initialize schema
    if (!initializeSchema()) {
        std::cerr << "[DB] Failed to initialize schema" << std::endl;
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

DatabaseManager::~DatabaseManager() {
    if (db_) {
        sqlite3_close(db_);
        std::cout << "[DB] Database closed" << std::endl;
    }
}

bool DatabaseManager::initializeSchema() {
    std::cout << "[DB] Initializing database schema..." << std::endl;

    // Create users table
    const char* users_sql = R"(
        CREATE TABLE IF NOT EXISTS users (
            user_id INTEGER PRIMARY KEY AUTOINCREMENT,
            username TEXT UNIQUE NOT NULL,
            password_hash TEXT NOT NULL,
            display_name TEXT NOT NULL,
            elo_rating INTEGER DEFAULT 1000,
            created_at INTEGER NOT NULL,
            last_login INTEGER
        );
    )";

    // Create sessions table
    const char* sessions_sql = R"(
        CREATE TABLE IF NOT EXISTS sessions (
            session_id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id INTEGER NOT NULL,
            session_token TEXT UNIQUE NOT NULL,
            created_at INTEGER NOT NULL,
            expires_at INTEGER NOT NULL,
            FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE CASCADE
        );
    )";

    // Create matches table
    const char* matches_sql = R"(
        CREATE TABLE IF NOT EXISTS matches (
            match_id INTEGER PRIMARY KEY AUTOINCREMENT,
            player1_id INTEGER NOT NULL,
            player2_id INTEGER NOT NULL,
            winner_id INTEGER,
            status TEXT NOT NULL DEFAULT 'waiting',
            created_at INTEGER NOT NULL,
            ended_at INTEGER,
            FOREIGN KEY (player1_id) REFERENCES users(user_id),
            FOREIGN KEY (player2_id) REFERENCES users(user_id),
            FOREIGN KEY (winner_id) REFERENCES users(user_id)
        );
    )";

    // Create match_boards table
    const char* boards_sql = R"(
        CREATE TABLE IF NOT EXISTS match_boards (
            board_id INTEGER PRIMARY KEY AUTOINCREMENT,
            match_id INTEGER NOT NULL,
            user_id INTEGER NOT NULL,
            ship_data TEXT NOT NULL,
            FOREIGN KEY (match_id) REFERENCES matches(match_id) ON DELETE CASCADE,
            FOREIGN KEY (user_id) REFERENCES users(user_id)
        );
    )";

    // Create match_moves table
    const char* moves_sql = R"(
        CREATE TABLE IF NOT EXISTS match_moves (
            move_id INTEGER PRIMARY KEY AUTOINCREMENT,
            match_id INTEGER NOT NULL,
            player_id INTEGER NOT NULL,
            move_number INTEGER NOT NULL,
            x INTEGER NOT NULL,
            y INTEGER NOT NULL,
            result TEXT NOT NULL,
            timestamp INTEGER NOT NULL,
            FOREIGN KEY (match_id) REFERENCES matches(match_id) ON DELETE CASCADE,
            FOREIGN KEY (player_id) REFERENCES users(user_id)
        );
    )";

    // Create indexes for performance
    const char* indexes_sql = R"(
        CREATE INDEX IF NOT EXISTS idx_sessions_token ON sessions(session_token);
        CREATE INDEX IF NOT EXISTS idx_sessions_user ON sessions(user_id);
        CREATE INDEX IF NOT EXISTS idx_matches_players ON matches(player1_id, player2_id);
        CREATE INDEX IF NOT EXISTS idx_moves_match ON match_moves(match_id);
    )";

    // Execute all schema creation
    return executeSQL(users_sql) &&
           executeSQL(sessions_sql) &&
           executeSQL(matches_sql) &&
           executeSQL(boards_sql) &&
           executeSQL(moves_sql) &&
           executeSQL(indexes_sql);
}

bool DatabaseManager::executeSQL(const std::string& sql) {
    char* err_msg = nullptr;
    int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &err_msg);

    if (rc != SQLITE_OK) {
        last_error_ = std::string(err_msg);
        std::cerr << "[DB] SQL error: " << last_error_ << std::endl;
        sqlite3_free(err_msg);
        return false;
    }

    return true;
}

sqlite3_stmt* DatabaseManager::prepareStatement(const std::string& sql) {
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        last_error_ = sqlite3_errmsg(db_);
        std::cerr << "[DB] Prepare error: " << last_error_ << std::endl;
        return nullptr;
    }

    return stmt;
}

// ===== USER OPERATIONS =====

uint32_t DatabaseManager::createUser(const std::string& username,
                                     const std::string& password_hash,
                                     const std::string& display_name) {
    if (!db_) return 0;

    const char* sql = "INSERT INTO users (username, password_hash, display_name, created_at) "
                      "VALUES (?, ?, ?, ?);";

    sqlite3_stmt* stmt = prepareStatement(sql);
    if (!stmt) return 0;

    time_t now = time(nullptr);

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, password_hash.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, display_name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 4, now);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        last_error_ = sqlite3_errmsg(db_);
        std::cerr << "[DB] Create user failed: " << last_error_ << std::endl;
        return 0;
    }

    uint32_t user_id = static_cast<uint32_t>(sqlite3_last_insert_rowid(db_));
    std::cout << "[DB] Created user: " << username << " (ID: " << user_id << ")" << std::endl;

    return user_id;
}

User DatabaseManager::getUserByUsername(const std::string& username) {
    User user;
    if (!db_) return user;

    const char* sql = "SELECT user_id, username, password_hash, display_name, "
                      "elo_rating, created_at, last_login FROM users WHERE username = ?;";

    sqlite3_stmt* stmt = prepareStatement(sql);
    if (!stmt) return user;

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        user.user_id = sqlite3_column_int(stmt, 0);
        user.username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        user.password_hash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        user.display_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        user.elo_rating = sqlite3_column_int(stmt, 4);
        user.created_at = sqlite3_column_int64(stmt, 5);
        user.last_login = sqlite3_column_int64(stmt, 6);
    }

    sqlite3_finalize(stmt);
    return user;
}

User DatabaseManager::getUserById(uint32_t user_id) {
    User user;
    if (!db_) return user;

    const char* sql = "SELECT user_id, username, password_hash, display_name, "
                      "elo_rating, created_at, last_login FROM users WHERE user_id = ?;";

    sqlite3_stmt* stmt = prepareStatement(sql);
    if (!stmt) return user;

    sqlite3_bind_int(stmt, 1, user_id);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        user.user_id = sqlite3_column_int(stmt, 0);
        user.username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        user.password_hash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        user.display_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        user.elo_rating = sqlite3_column_int(stmt, 4);
        user.created_at = sqlite3_column_int64(stmt, 5);
        user.last_login = sqlite3_column_int64(stmt, 6);
    }

    sqlite3_finalize(stmt);
    return user;
}

bool DatabaseManager::updateLastLogin(uint32_t user_id) {
    if (!db_) return false;

    const char* sql = "UPDATE users SET last_login = ? WHERE user_id = ?;";

    sqlite3_stmt* stmt = prepareStatement(sql);
    if (!stmt) return false;

    time_t now = time(nullptr);
    sqlite3_bind_int64(stmt, 1, now);
    sqlite3_bind_int(stmt, 2, user_id);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return rc == SQLITE_DONE;
}

bool DatabaseManager::updateEloRating(uint32_t user_id, int32_t new_elo) {
    if (!db_) return false;

    const char* sql = "UPDATE users SET elo_rating = ? WHERE user_id = ?;";

    sqlite3_stmt* stmt = prepareStatement(sql);
    if (!stmt) return false;

    sqlite3_bind_int(stmt, 1, new_elo);
    sqlite3_bind_int(stmt, 2, user_id);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return rc == SQLITE_DONE;
}

bool DatabaseManager::usernameExists(const std::string& username) {
    if (!db_) return false;

    const char* sql = "SELECT COUNT(*) FROM users WHERE username = ?;";

    sqlite3_stmt* stmt = prepareStatement(sql);
    if (!stmt) return false;

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);

    bool exists = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        exists = sqlite3_column_int(stmt, 0) > 0;
    }

    sqlite3_finalize(stmt);
    return exists;
}

// ===== SESSION OPERATIONS =====

uint32_t DatabaseManager::createSession(uint32_t user_id,
                                        const std::string& session_token,
                                        int duration_hours) {
    if (!db_) return 0;

    const char* sql = "INSERT INTO sessions (user_id, session_token, created_at, expires_at) "
                      "VALUES (?, ?, ?, ?);";

    sqlite3_stmt* stmt = prepareStatement(sql);
    if (!stmt) return 0;

    time_t now = time(nullptr);
    time_t expires = now + (duration_hours * 3600);

    sqlite3_bind_int(stmt, 1, user_id);
    sqlite3_bind_text(stmt, 2, session_token.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 3, now);
    sqlite3_bind_int64(stmt, 4, expires);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        last_error_ = sqlite3_errmsg(db_);
        std::cerr << "[DB] Create session failed: " << last_error_ << std::endl;
        return 0;
    }

    uint32_t session_id = static_cast<uint32_t>(sqlite3_last_insert_rowid(db_));
    std::cout << "[DB] Created session for user " << user_id << " (expires in "
              << duration_hours << "h)" << std::endl;

    return session_id;
}

Session DatabaseManager::getSessionByToken(const std::string& session_token) {
    Session session;
    if (!db_) return session;

    const char* sql = "SELECT session_id, user_id, session_token, created_at, expires_at "
                      "FROM sessions WHERE session_token = ?;";

    sqlite3_stmt* stmt = prepareStatement(sql);
    if (!stmt) return session;

    sqlite3_bind_text(stmt, 1, session_token.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        session.session_id = sqlite3_column_int(stmt, 0);
        session.user_id = sqlite3_column_int(stmt, 1);
        session.session_token = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        session.created_at = sqlite3_column_int64(stmt, 3);
        session.expires_at = sqlite3_column_int64(stmt, 4);
    }

    sqlite3_finalize(stmt);
    return session;
}

uint32_t DatabaseManager::validateSession(const std::string& session_token) {
    Session session = getSessionByToken(session_token);

    if (session.session_id == 0) {
        return 0; // Session not found
    }

    if (session.isExpired()) {
        std::cout << "[DB] Session expired: " << session_token << std::endl;
        deleteSession(session_token); // Clean up expired session
        return 0;
    }

    return session.user_id;
}

bool DatabaseManager::deleteSession(const std::string& session_token) {
    if (!db_) return false;

    const char* sql = "DELETE FROM sessions WHERE session_token = ?;";

    sqlite3_stmt* stmt = prepareStatement(sql);
    if (!stmt) return false;

    sqlite3_bind_text(stmt, 1, session_token.c_str(), -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc == SQLITE_DONE) {
        std::cout << "[DB] Deleted session: " << session_token << std::endl;
        return true;
    }

    return false;
}

int DatabaseManager::cleanupExpiredSessions() {
    if (!db_) return 0;

    const char* sql = "DELETE FROM sessions WHERE expires_at < ?;";

    sqlite3_stmt* stmt = prepareStatement(sql);
    if (!stmt) return 0;

    time_t now = time(nullptr);
    sqlite3_bind_int64(stmt, 1, now);

    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    int deleted = sqlite3_changes(db_);
    if (deleted > 0) {
        std::cout << "[DB] Cleaned up " << deleted << " expired sessions" << std::endl;
    }

    return deleted;
}

bool DatabaseManager::deleteUserSessions(uint32_t user_id) {
    if (!db_) return false;

    const char* sql = "DELETE FROM sessions WHERE user_id = ?;";

    sqlite3_stmt* stmt = prepareStatement(sql);
    if (!stmt) return false;

    sqlite3_bind_int(stmt, 1, user_id);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return rc == SQLITE_DONE;
}

// ===== MATCH OPERATIONS (for Phase 4-5) =====

uint32_t DatabaseManager::createMatch(uint32_t player1_id, uint32_t player2_id) {
    if (!db_) return 0;

    const char* sql = "INSERT INTO matches (player1_id, player2_id, status, created_at) "
                      "VALUES (?, ?, 'waiting', ?);";

    sqlite3_stmt* stmt = prepareStatement(sql);
    if (!stmt) return 0;

    time_t now = time(nullptr);

    sqlite3_bind_int(stmt, 1, player1_id);
    sqlite3_bind_int(stmt, 2, player2_id);
    sqlite3_bind_int64(stmt, 3, now);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        last_error_ = sqlite3_errmsg(db_);
        return 0;
    }

    return static_cast<uint32_t>(sqlite3_last_insert_rowid(db_));
}

Match DatabaseManager::getMatchById(uint32_t match_id) {
    Match match;
    if (!db_) return match;

    const char* sql = "SELECT match_id, player1_id, player2_id, winner_id, status, "
                      "created_at, ended_at FROM matches WHERE match_id = ?;";

    sqlite3_stmt* stmt = prepareStatement(sql);
    if (!stmt) return match;

    sqlite3_bind_int(stmt, 1, match_id);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        match.match_id = sqlite3_column_int(stmt, 0);
        match.player1_id = sqlite3_column_int(stmt, 1);
        match.player2_id = sqlite3_column_int(stmt, 2);
        match.winner_id = sqlite3_column_int(stmt, 3);
        match.status = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        match.created_at = sqlite3_column_int64(stmt, 5);
        match.ended_at = sqlite3_column_int64(stmt, 6);
    }

    sqlite3_finalize(stmt);
    return match;
}

bool DatabaseManager::updateMatchStatus(uint32_t match_id, const std::string& status) {
    if (!db_) return false;

    const char* sql = "UPDATE matches SET status = ? WHERE match_id = ?;";

    sqlite3_stmt* stmt = prepareStatement(sql);
    if (!stmt) return false;

    sqlite3_bind_text(stmt, 1, status.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, match_id);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return rc == SQLITE_DONE;
}

bool DatabaseManager::endMatch(uint32_t match_id, uint32_t winner_id) {
    if (!db_) return false;

    const char* sql = "UPDATE matches SET status = 'completed', winner_id = ?, ended_at = ? "
                      "WHERE match_id = ?;";

    sqlite3_stmt* stmt = prepareStatement(sql);
    if (!stmt) return false;

    time_t now = time(nullptr);

    sqlite3_bind_int(stmt, 1, winner_id);
    sqlite3_bind_int64(stmt, 2, now);
    sqlite3_bind_int(stmt, 3, match_id);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return rc == SQLITE_DONE;
}

std::vector<Match> DatabaseManager::getUserMatches(uint32_t user_id, int limit) {
    std::vector<Match> matches;
    if (!db_) return matches;

    const char* sql = "SELECT match_id, player1_id, player2_id, winner_id, status, "
                      "created_at, ended_at FROM matches "
                      "WHERE player1_id = ? OR player2_id = ? "
                      "ORDER BY created_at DESC LIMIT ?;";

    sqlite3_stmt* stmt = prepareStatement(sql);
    if (!stmt) return matches;

    sqlite3_bind_int(stmt, 1, user_id);
    sqlite3_bind_int(stmt, 2, user_id);
    sqlite3_bind_int(stmt, 3, limit);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Match match;
        match.match_id = sqlite3_column_int(stmt, 0);
        match.player1_id = sqlite3_column_int(stmt, 1);
        match.player2_id = sqlite3_column_int(stmt, 2);
        match.winner_id = sqlite3_column_int(stmt, 3);
        match.status = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        match.created_at = sqlite3_column_int64(stmt, 5);
        match.ended_at = sqlite3_column_int64(stmt, 6);
        matches.push_back(match);
    }

    sqlite3_finalize(stmt);
    return matches;
}

// ===== BOARD OPERATIONS (for Phase 4-5) =====

bool DatabaseManager::saveShipPlacement(uint32_t match_id, uint32_t user_id,
                                       const std::string& ship_data) {
    if (!db_) return false;

    const char* sql = "INSERT INTO match_boards (match_id, user_id, ship_data) "
                      "VALUES (?, ?, ?);";

    sqlite3_stmt* stmt = prepareStatement(sql);
    if (!stmt) return false;

    sqlite3_bind_int(stmt, 1, match_id);
    sqlite3_bind_int(stmt, 2, user_id);
    sqlite3_bind_text(stmt, 3, ship_data.c_str(), -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return rc == SQLITE_DONE;
}

std::string DatabaseManager::getShipPlacement(uint32_t match_id, uint32_t user_id) {
    std::string ship_data;
    if (!db_) return ship_data;

    const char* sql = "SELECT ship_data FROM match_boards "
                      "WHERE match_id = ? AND user_id = ?;";

    sqlite3_stmt* stmt = prepareStatement(sql);
    if (!stmt) return ship_data;

    sqlite3_bind_int(stmt, 1, match_id);
    sqlite3_bind_int(stmt, 2, user_id);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        ship_data = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
    }

    sqlite3_finalize(stmt);
    return ship_data;
}

// ===== MOVE OPERATIONS (for Phase 5) =====

bool DatabaseManager::saveMove(uint32_t match_id, uint32_t player_id,
                               int move_number, int x, int y,
                               const std::string& result) {
    if (!db_) return false;

    const char* sql = "INSERT INTO match_moves (match_id, player_id, move_number, x, y, result, timestamp) "
                      "VALUES (?, ?, ?, ?, ?, ?, ?);";

    sqlite3_stmt* stmt = prepareStatement(sql);
    if (!stmt) return false;

    time_t now = time(nullptr);

    sqlite3_bind_int(stmt, 1, match_id);
    sqlite3_bind_int(stmt, 2, player_id);
    sqlite3_bind_int(stmt, 3, move_number);
    sqlite3_bind_int(stmt, 4, x);
    sqlite3_bind_int(stmt, 5, y);
    sqlite3_bind_text(stmt, 6, result.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 7, now);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return rc == SQLITE_DONE;
}

std::vector<std::string> DatabaseManager::getMatchMoves(uint32_t match_id) {
    std::vector<std::string> moves;
    if (!db_) return moves;

    const char* sql = "SELECT player_id, move_number, x, y, result, timestamp "
                      "FROM match_moves WHERE match_id = ? ORDER BY move_number;";

    sqlite3_stmt* stmt = prepareStatement(sql);
    if (!stmt) return moves;

    sqlite3_bind_int(stmt, 1, match_id);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        std::stringstream ss;
        ss << sqlite3_column_int(stmt, 0) << ","  // player_id
           << sqlite3_column_int(stmt, 1) << ","  // move_number
           << sqlite3_column_int(stmt, 2) << ","  // x
           << sqlite3_column_int(stmt, 3) << ","  // y
           << reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4)) << ","  // result
           << sqlite3_column_int64(stmt, 5);      // timestamp
        moves.push_back(ss.str());
    }

    sqlite3_finalize(stmt);
    return moves;
}
