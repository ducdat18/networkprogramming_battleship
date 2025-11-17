#include "session_storage.h"
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>

std::string SessionStorage::getSessionFilePath() {
    // Get home directory
    const char* home_dir = getenv("HOME");
    if (!home_dir) {
        struct passwd* pw = getpwuid(getuid());
        home_dir = pw->pw_dir;
    }

    std::string battleship_dir = std::string(home_dir) + "/.battleship";

    // Create directory if doesn't exist
    mkdir(battleship_dir.c_str(), 0700);

    return battleship_dir + "/session.txt";
}

bool SessionStorage::saveSession(uint32_t user_id,
                                 const std::string& session_token,
                                 const std::string& username,
                                 const std::string& display_name,
                                 int32_t elo_rating) {
    std::string file_path = getSessionFilePath();
    std::ofstream file(file_path);

    if (!file.is_open()) {
        return false;
    }

    // Write session data (one value per line)
    file << user_id << std::endl;
    file << session_token << std::endl;
    file << username << std::endl;
    file << display_name << std::endl;
    file << elo_rating << std::endl;

    file.close();

    // Set file permissions to 600 (owner read/write only)
    chmod(file_path.c_str(), 0600);

    return true;
}

bool SessionStorage::loadSession(uint32_t& user_id,
                                 std::string& session_token,
                                 std::string& username,
                                 std::string& display_name,
                                 int32_t& elo_rating) {
    std::string file_path = getSessionFilePath();
    std::ifstream file(file_path);

    if (!file.is_open()) {
        return false;
    }

    // Read session data
    std::string line;

    if (!std::getline(file, line)) return false;
    user_id = std::stoi(line);

    if (!std::getline(file, session_token)) return false;
    if (!std::getline(file, username)) return false;
    if (!std::getline(file, display_name)) return false;

    if (!std::getline(file, line)) return false;
    elo_rating = std::stoi(line);

    file.close();
    return true;
}

void SessionStorage::clearSession() {
    std::string file_path = getSessionFilePath();
    unlink(file_path.c_str());
}

bool SessionStorage::hasStoredSession() {
    std::string file_path = getSessionFilePath();
    std::ifstream file(file_path);
    return file.good();
}
