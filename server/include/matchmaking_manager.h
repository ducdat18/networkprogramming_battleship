#ifndef MATCHMAKING_MANAGER_H
#define MATCHMAKING_MANAGER_H

#include <deque>
#include <map>
#include <mutex>
#include <thread>
#include <atomic>
#include <ctime>
#include "protocol.h"
#include "messages/matchmaking_messages.h"

class Server;
class PlayerManager;
class DatabaseManager;

/**
 * MatchmakingManager - Queue-based ELO matchmaking
 *
 * Algorithm:
 * - Players join queue with ELO rating
 * - Initial search range: ±200 ELO
 * - Range expands +100 every 30 seconds
 * - Match found: create match, send MATCH_START to both
 * - Thread-safe queue operations
 */
class MatchmakingManager {
public:
    MatchmakingManager(Server* server, PlayerManager* player_manager,
                      DatabaseManager* db);
    ~MatchmakingManager();

    // Queue operations
    bool joinQueue(uint32_t user_id, uint32_t time_limit);
    bool leaveQueue(uint32_t user_id);
    bool isInQueue(uint32_t user_id) const;

    QueueStatusMessage getQueueStatus(uint32_t user_id) const;

    // Lifecycle
    void start();
    void stop();

private:
    struct QueueEntry {
        uint32_t user_id;
        int32_t elo_rating;
        uint32_t time_limit;
        time_t join_time;

        QueueEntry() : user_id(0), elo_rating(1000), time_limit(60), join_time(0) {}
    };

    Server* server_;
    PlayerManager* player_manager_;
    DatabaseManager* db_;

    mutable std::mutex queue_mutex_;
    std::deque<QueueEntry> queue_;
    std::map<uint32_t, size_t> user_queue_index_;

    std::thread matchmaking_thread_;
    std::atomic<bool> running_;

    void matchmakingLoop();
    bool tryMatchPlayers();
    int32_t getEloRange(time_t join_time) const;
    void createMatch(const QueueEntry& p1, const QueueEntry& p2);

    static constexpr int32_t INITIAL_ELO_RANGE = 200;
    static constexpr int32_t ELO_RANGE_EXPANSION = 100;
    static constexpr time_t EXPANSION_INTERVAL_SECONDS = 30;
    static constexpr int TICK_INTERVAL_MS = 2000;
};

#endif // MATCHMAKING_MANAGER_H
