#include "matchmaking_manager.h"
#include "server.h"
#include "player_manager.h"
#include "database.h"
#include "client_connection.h"
#include "messages/matchmaking_messages.h"
#include <iostream>
#include <cstring>
#include <algorithm>
#include <chrono>

MatchmakingManager::MatchmakingManager(Server* server,
                                       PlayerManager* player_manager,
                                       DatabaseManager* db)
    : server_(server), player_manager_(player_manager), db_(db), running_(false) {
}

MatchmakingManager::~MatchmakingManager() {
    stop();
}

void MatchmakingManager::start() {
    if (running_.load()) return;

    running_.store(true);
    matchmaking_thread_ = std::thread(&MatchmakingManager::matchmakingLoop, this);
    std::cout << "[MATCHMAKING] Started" << std::endl;
}

void MatchmakingManager::stop() {
    if (!running_.load()) return;

    running_.store(false);
    if (matchmaking_thread_.joinable()) {
        matchmaking_thread_.join();
    }
    std::cout << "[MATCHMAKING] Stopped" << std::endl;
}

bool MatchmakingManager::joinQueue(uint32_t user_id, uint32_t time_limit) {
    std::lock_guard<std::mutex> lock(queue_mutex_);

    // Check if already in queue
    if (user_queue_index_.find(user_id) != user_queue_index_.end()) {
        std::cout << "[MATCHMAKING] User " << user_id << " already in queue" << std::endl;
        return false;
    }

    // Get user's ELO
    User user = db_->getUserById(user_id);
    if (user.user_id == 0) {
        std::cerr << "[MATCHMAKING] User not found: " << user_id << std::endl;
        return false;
    }

    QueueEntry entry;
    entry.user_id = user_id;
    entry.elo_rating = user.elo_rating;
    entry.time_limit = time_limit;
    entry.join_time = time(nullptr);

    queue_.push_back(entry);
    user_queue_index_[user_id] = queue_.size() - 1;

    std::cout << "[MATCHMAKING] User " << user_id << " joined queue (ELO: "
              << entry.elo_rating << ", queue size: " << queue_.size() << ")" << std::endl;

    // Update player status
    player_manager_->updatePlayerStatus(user_id, STATUS_BUSY);

    return true;
}

bool MatchmakingManager::leaveQueue(uint32_t user_id) {
    std::lock_guard<std::mutex> lock(queue_mutex_);

    auto it = user_queue_index_.find(user_id);
    if (it == user_queue_index_.end()) {
        return false;
    }

    size_t index = it->second;
    queue_.erase(queue_.begin() + index);
    user_queue_index_.erase(it);

    // Rebuild index map
    for (size_t i = 0; i < queue_.size(); ++i) {
        user_queue_index_[queue_[i].user_id] = i;
    }

    std::cout << "[MATCHMAKING] User " << user_id << " left queue (queue size: "
              << queue_.size() << ")" << std::endl;

    // Update player status back to available
    player_manager_->updatePlayerStatus(user_id, STATUS_AVAILABLE);

    return true;
}

bool MatchmakingManager::isInQueue(uint32_t user_id) const {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    return user_queue_index_.find(user_id) != user_queue_index_.end();
}

QueueStatusMessage MatchmakingManager::getQueueStatus(uint32_t user_id) const {
    std::lock_guard<std::mutex> lock(queue_mutex_);

    QueueStatusMessage status;

    auto it = user_queue_index_.find(user_id);
    if (it == user_queue_index_.end()) {
        status.in_queue = false;
        return status;
    }

    status.in_queue = true;
    status.queue_position = it->second + 1;
    status.players_in_queue = queue_.size();

    const QueueEntry& entry = queue_[it->second];
    time_t now = time(nullptr);
    status.wait_time_seconds = now - entry.join_time;

    int32_t range = getEloRange(entry.join_time);
    status.elo_range_min = entry.elo_rating - range;
    status.elo_range_max = entry.elo_rating + range;

    return status;
}

int32_t MatchmakingManager::getEloRange(time_t join_time) const {
    time_t now = time(nullptr);
    time_t elapsed = now - join_time;
    int32_t expansions = elapsed / EXPANSION_INTERVAL_SECONDS;
    return INITIAL_ELO_RANGE + (expansions * ELO_RANGE_EXPANSION);
}

void MatchmakingManager::matchmakingLoop() {
    while (running_.load()) {
        tryMatchPlayers();
        std::this_thread::sleep_for(std::chrono::milliseconds(TICK_INTERVAL_MS));
    }
}

bool MatchmakingManager::tryMatchPlayers() {
    std::unique_lock<std::mutex> lock(queue_mutex_);

    if (queue_.size() < 2) return false;

    const QueueEntry& p1 = queue_.front();
    int32_t p1_range = getEloRange(p1.join_time);
    int32_t p1_min = p1.elo_rating - p1_range;
    int32_t p1_max = p1.elo_rating + p1_range;

    for (size_t i = 1; i < queue_.size(); ++i) {
        const QueueEntry& p2 = queue_[i];
        int32_t p2_range = getEloRange(p2.join_time);
        int32_t p2_min = p2.elo_rating - p2_range;
        int32_t p2_max = p2.elo_rating + p2_range;

        // Check if ranges overlap
        bool overlap = !(p1_max < p2_min || p2_max < p1_min);

        if (overlap) {
            std::cout << "[MATCHMAKING] Match found! User " << p1.user_id
                      << " (ELO " << p1.elo_rating << ") vs User " << p2.user_id
                      << " (ELO " << p2.elo_rating << ")" << std::endl;

            QueueEntry match_p1 = p1;
            QueueEntry match_p2 = p2;

            // Remove from queue
            queue_.erase(queue_.begin() + i);
            queue_.pop_front();

            // Rebuild index
            user_queue_index_.clear();
            for (size_t j = 0; j < queue_.size(); ++j) {
                user_queue_index_[queue_[j].user_id] = j;
            }

            // Unlock before creating match to avoid deadlock
            lock.unlock();
            createMatch(match_p1, match_p2);

            return true;
        }
    }

    return false;
}

void MatchmakingManager::createMatch(const QueueEntry& p1, const QueueEntry& p2) {
    // Create match in database
    uint32_t match_id = db_->createMatch(p1.user_id, p2.user_id);

    if (match_id == 0) {
        std::cerr << "[MATCHMAKING] Failed to create match" << std::endl;
        // Return players to available status
        player_manager_->updatePlayerStatus(p1.user_id, STATUS_AVAILABLE);
        player_manager_->updatePlayerStatus(p2.user_id, STATUS_AVAILABLE);
        return;
    }

    // Update statuses
    player_manager_->updatePlayerStatus(p1.user_id, STATUS_IN_GAME);
    player_manager_->updatePlayerStatus(p2.user_id, STATUS_IN_GAME);

    // Get user info
    User user1 = db_->getUserById(p1.user_id);
    User user2 = db_->getUserById(p2.user_id);

    // Send MATCH_START to player 1
    MatchStartMessage start_msg1;
    start_msg1.match_id = match_id;
    start_msg1.opponent_id = p2.user_id;
    strncpy(start_msg1.opponent_name, user2.display_name.c_str(), 63);
    start_msg1.opponent_name[63] = '\0';
    start_msg1.opponent_elo = user2.elo_rating;
    start_msg1.time_limit = std::min(p1.time_limit, p2.time_limit);
    start_msg1.you_go_first = true; // P1 goes first

    MessageHeader header;
    memset(&header, 0, sizeof(header));
    header.type = MessageType::MATCH_START;
    header.length = sizeof(start_msg1);
    header.timestamp = time(nullptr);

    ClientConnection* conn1 = player_manager_->getClientConnection(p1.user_id);
    if (conn1) {
        server_->sendToClient(conn1->getSocketFd(), header, &start_msg1, sizeof(start_msg1));
    }

    // Send MATCH_START to player 2
    MatchStartMessage start_msg2 = start_msg1;
    start_msg2.opponent_id = p1.user_id;
    strncpy(start_msg2.opponent_name, user1.display_name.c_str(), 63);
    start_msg2.opponent_name[63] = '\0';
    start_msg2.opponent_elo = user1.elo_rating;
    start_msg2.you_go_first = false; // P2 goes second

    ClientConnection* conn2 = player_manager_->getClientConnection(p2.user_id);
    if (conn2) {
        server_->sendToClient(conn2->getSocketFd(), header, &start_msg2, sizeof(start_msg2));
    }

    std::cout << "[MATCHMAKING] Match " << match_id << " created successfully" << std::endl;
}
