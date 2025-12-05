/**
 * Manual Test: Lobby Flow (Player List + Challenge)
 *
 * This test demonstrates the full lobby flow:
 * 1. Two clients connect and login
 * 2. Both request player list
 * 3. Client 1 challenges Client 2
 * 4. Client 2 accepts
 * 5. Match is created
 *
 * Run with:
 *   Terminal 1: ./bin/battleship_server 9999
 *   Terminal 2: ./bin/test_lobby_flow
 */

#include <iostream>
#include <thread>
#include <chrono>
#include "client_network.h"
#include "config.h"

using namespace std;

void test_client(int client_num, const string& username) {
    cout << "\n[Client " << client_num << "] Starting..." << endl;

    ClientNetwork client;

    // Connect to server
    cout << "[Client " << client_num << "] Connecting to server..." << endl;
    if (!client.connect(SERVER_HOST, SERVER_PORT)) {
        cerr << "[Client " << client_num << "] Failed to connect!" << endl;
        return;
    }

    this_thread::sleep_for(chrono::milliseconds(500));

    // Register
    cout << "[Client " << client_num << "] Registering as " << username << "..." << endl;
    bool register_done = false;
    client.registerUser(username, "password123", "Player " + to_string(client_num),
        [&](bool success, uint32_t user_id, const string& error) {
            if (success) {
                cout << "[Client " << client_num << "] ✅ Registered! user_id=" << user_id << endl;
            } else {
                cout << "[Client " << client_num << "] ❌ Register failed: " << error << endl;
            }
            register_done = true;
        });

    while (!register_done) {
        this_thread::sleep_for(chrono::milliseconds(100));
    }

    // Login
    cout << "[Client " << client_num << "] Logging in..." << endl;
    bool login_done = false;
    uint32_t my_user_id = 0;
    client.loginUser(username, "password123",
        [&](bool success, uint32_t user_id, const string& display_name,
            int32_t elo_rating, const string& session_token, const string& error) {
            if (success) {
                cout << "[Client " << client_num << "] ✅ Logged in! user_id=" << user_id
                     << " elo=" << elo_rating << endl;
                my_user_id = user_id;
            } else {
                cout << "[Client " << client_num << "] ❌ Login failed: " << error << endl;
            }
            login_done = true;
        });

    while (!login_done) {
        this_thread::sleep_for(chrono::milliseconds(100));
    }

    // Set up challenge received callback
    client.setChallengeReceivedCallback([&](const ChallengeReceived& challenge) {
        cout << "\n[Client " << client_num << "] 📨 CHALLENGE RECEIVED!" << endl;
        cout << "  From: " << challenge.challenger_name << " (ELO " << challenge.challenger_elo << ")" << endl;
        cout << "  Challenge ID: " << challenge.challenge_id << endl;

        // Auto-accept after 1 second
        this_thread::sleep_for(chrono::seconds(1));
        cout << "[Client " << client_num << "] ✅ Accepting challenge..." << endl;
        client.respondToChallenge(challenge.challenge_id, true);
    });

    // Set up match start callback
    client.setMatchStartCallback([&](const MatchStartMessage& match) {
        cout << "\n[Client " << client_num << "] 🎮 MATCH STARTED!" << endl;
        cout << "  Match ID: " << match.match_id << endl;
        cout << "  Opponent: " << match.opponent_name << " (ELO " << match.opponent_elo << ")" << endl;
        cout << "  You go first: " << (match.you_go_first ? "Yes" : "No") << endl;
    });

    // Wait for other client to connect
    this_thread::sleep_for(chrono::seconds(2));

    // Request player list
    cout << "[Client " << client_num << "] Requesting player list..." << endl;
    bool player_list_done = false;
    uint32_t other_user_id = 0;
    client.requestPlayerList([&](bool success, const vector<PlayerInfo_Message>& players) {
        if (success) {
            cout << "[Client " << client_num << "] ✅ Player list received (" << players.size() << " players):" << endl;
            for (const auto& player : players) {
                cout << "  - " << player.display_name
                     << " (ID=" << player.user_id
                     << ", ELO=" << player.elo_rating
                     << ", Status=" << (int)player.status << ")" << endl;

                // Store other player's ID
                if (player.user_id != my_user_id) {
                    other_user_id = player.user_id;
                }
            }
        } else {
            cout << "[Client " << client_num << "] ❌ Failed to get player list" << endl;
        }
        player_list_done = true;
    });

    while (!player_list_done) {
        this_thread::sleep_for(chrono::milliseconds(100));
    }

    // Client 1 sends challenge
    if (client_num == 1 && other_user_id > 0) {
        this_thread::sleep_for(chrono::seconds(1));
        cout << "\n[Client " << client_num << "] 💪 Sending challenge to user " << other_user_id << "..." << endl;

        bool challenge_done = false;
        client.sendChallenge(other_user_id, 60, false, [&](bool success, const string& error) {
            if (success) {
                cout << "[Client " << client_num << "] ✅ Challenge sent successfully!" << endl;
            } else {
                cout << "[Client " << client_num << "] ❌ Failed to send challenge: " << error << endl;
            }
            challenge_done = true;
        });

        while (!challenge_done) {
            this_thread::sleep_for(chrono::milliseconds(100));
        }
    }

    // Keep running to receive messages
    cout << "[Client " << client_num << "] Waiting for events..." << endl;
    this_thread::sleep_for(chrono::seconds(5));

    cout << "[Client " << client_num << "] Test complete!" << endl;
}

int main() {
    cout << "╔════════════════════════════════════════════╗" << endl;
    cout << "║   LOBBY FLOW MANUAL TEST                   ║" << endl;
    cout << "╠════════════════════════════════════════════╣" << endl;
    cout << "║  Prerequisites:                            ║" << endl;
    cout << "║  - Server running on localhost:9999        ║" << endl;
    cout << "╚════════════════════════════════════════════╝" << endl;

    // Generate unique usernames with timestamp
    time_t now = time(nullptr);
    string user1 = "lobby_test1_" + to_string(now);
    string user2 = "lobby_test2_" + to_string(now);

    // Run two clients in parallel
    thread t1(test_client, 1, user1);
    thread t2(test_client, 2, user2);

    t1.join();
    t2.join();

    cout << "\n✅ Test completed!" << endl;
    return 0;
}
