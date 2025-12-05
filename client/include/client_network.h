#ifndef CLIENT_NETWORK_H
#define CLIENT_NETWORK_H

#include <string>
#include <functional>
#include <thread>
#include <mutex>
#include <atomic>
#include <queue>
#include <vector>
#include "protocol.h"
#include "messages/authentication_messages.h"
#include "messages/matchmaking_messages.h"

/**
 * Client Network Manager
 * Handles connection to server and message exchange
 *
 * Features:
 * - Socket connection management
 * - Async message sending/receiving
 * - Authentication API
 * - Callback system for responses
 * - Thread-safe operations
 */
class ClientNetwork {
public:
    // Connection status
    enum ConnectionStatus {
        DISCONNECTED,
        CONNECTING,
        CONNECTED,
        AUTHENTICATION_PENDING,
        AUTHENTICATED,
        ERROR_STATE
    };

    // Response callbacks
    using RegisterCallback = std::function<void(bool success, uint32_t user_id, const std::string& error)>;
    using LoginCallback = std::function<void(bool success, uint32_t user_id, const std::string& display_name,
                                             int32_t elo_rating, const std::string& session_token, const std::string& error)>;
    using LogoutCallback = std::function<void(bool success)>;
    using ValidateSessionCallback = std::function<void(bool valid, uint32_t user_id, const std::string& username,
                                                       const std::string& display_name, int32_t elo_rating, const std::string& error)>;
    using ConnectionCallback = std::function<void(bool connected, const std::string& error)>;

    // Matchmaking callbacks
    using PlayerListCallback = std::function<void(bool success, const std::vector<PlayerInfo_Message>& players)>;
    using PlayerStatusCallback = std::function<void(const PlayerStatusUpdate& update)>;
    using SendChallengeCallback = std::function<void(bool success, const std::string& error)>;
    using ChallengeReceivedCallback = std::function<void(const ChallengeReceived& challenge)>;
    using MatchStartCallback = std::function<void(const MatchStartMessage& match)>;

    ClientNetwork();
    ~ClientNetwork();

    // Connection management
    bool connect(const std::string& host, int port, ConnectionCallback callback = nullptr);
    void disconnect();
    bool isConnected() const { return status_ == CONNECTED || status_ == AUTHENTICATED; }
    ConnectionStatus getStatus() const { return status_; }

    // Authentication API
    void registerUser(const std::string& username,
                     const std::string& password,
                     const std::string& display_name,
                     RegisterCallback callback);

    void loginUser(const std::string& username,
                  const std::string& password,
                  LoginCallback callback);

    void logoutUser(LogoutCallback callback);

    void validateSession(const std::string& session_token, ValidateSessionCallback callback);

    // Matchmaking API
    void requestPlayerList(PlayerListCallback callback);
    void sendChallenge(uint32_t target_user_id, uint32_t time_limit, bool random_placement, SendChallengeCallback callback);
    void respondToChallenge(uint32_t challenge_id, bool accept);

    // Event handlers (set these to receive notifications)
    void setPlayerStatusCallback(PlayerStatusCallback callback);
    void setChallengeReceivedCallback(ChallengeReceivedCallback callback);
    void setMatchStartCallback(MatchStartCallback callback);

    // Session info
    bool isAuthenticated() const { return status_ == AUTHENTICATED; }
    uint32_t getUserId() const { return user_id_; }
    const std::string& getSessionToken() const { return session_token_; }
    const std::string& getDisplayName() const { return display_name_; }
    int32_t getEloRating() const { return elo_rating_; }

private:
    // Socket operations
    bool connectSocket(const std::string& host, int port);
    void closeSocket();
    bool sendMessage(const MessageHeader& header, const std::string& payload);
    bool receiveMessage(MessageHeader& header, std::string& payload);

    // Message handling
    void receiveLoop();
    void handleAuthResponse(const std::string& payload);
    void handlePlayerListResponse(const std::string& payload);
    void handlePlayerStatusUpdate(const std::string& payload);
    void handleChallengeReceived(const std::string& payload);
    void handleMatchStart(const std::string& payload);

    // Connection state
    int socket_fd_;
    std::atomic<ConnectionStatus> status_;
    std::string host_;
    int port_;

    // Authentication state
    uint32_t user_id_;
    std::string session_token_;
    std::string display_name_;
    int32_t elo_rating_;

    // Async handling
    std::thread receive_thread_;
    std::atomic<bool> running_;

    // Callbacks
    std::mutex callback_mutex_;
    RegisterCallback register_callback_;
    LoginCallback login_callback_;
    LogoutCallback logout_callback_;
    ValidateSessionCallback validate_session_callback_;
    ConnectionCallback connection_callback_;

    // Matchmaking callbacks
    PlayerListCallback player_list_callback_;
    PlayerStatusCallback player_status_callback_;
    SendChallengeCallback send_challenge_callback_;
    ChallengeReceivedCallback challenge_received_callback_;
    MatchStartCallback match_start_callback_;

    // Pending request tracking
    enum PendingRequest {
        NONE,
        REGISTER,
        LOGIN,
        LOGOUT,
        VALIDATE_SESSION,
        PLAYER_LIST,
        SEND_CHALLENGE
    };
    std::atomic<PendingRequest> pending_request_;
};

#endif // CLIENT_NETWORK_H
