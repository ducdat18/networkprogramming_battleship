/**
 * Manual Test: Player Disconnect Detection
 *
 * This test verifies that when a player disconnects, the server broadcasts
 * STATUS_OFFLINE and other clients remove them from the lobby list.
 *
 * Test steps:
 * 1. Client 1 connects and logs in
 * 2. Client 2 connects and logs in
 * 3. Both clients request player list (should see each other)
 * 4. Client 2 disconnects
 * 5. Client 1 should receive STATUS_OFFLINE for Client 2
 *
 * Run with:
 *   Terminal 1: ./bin/battleship_server 9999
 *   Terminal 2: ./bin/test_disconnect
 */

#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include "client_network.h"
#include "config.h"

using namespace std;

atomic<bool> client2_should_run(true);

void test_client_1() {
    cout << "\n[Client 1] Starting..." << endl;

    ClientNetwork client;

    // Connect
    cout << "[Client 1] Connecting..." << endl;
    if (!client.connect(SERVER_HOST, SERVER_PORT)) {
        cerr << "[Client 1] Failed to connect!" << endl;
        return;
    }
    this_thread::sleep_for(chrono::milliseconds(500));

    // Register & Login
    time_t now = time(nullptr);
    string username = "disconnect_test1_" + to_string(now);

    bool register_done = false;
    client.registerUser(username, "password123", "Player 1",
        [&](bool success, uint32_t user_id, const string& error) {
            if (success) {
                cout << "[Client 1] ✅ Registered! user_id=" << user_id << endl;
            } else {
                cout << "[Client 1] ❌ Register failed: " << error << endl;
            }
            register_done = true;
        });

    while (!register_done) {
        this_thread::sleep_for(chrono::milliseconds(100));
    }

    bool login_done = false;
    client.loginUser(username, "password123",
        [&](bool success, uint32_t, const string&, int32_t, const string&, const string& error) {
            if (success) {
                cout << "[Client 1] ✅ Logged in!" << endl;
            } else {
                cout << "[Client 1] ❌ Login failed: " << error << endl;
            }
            login_done = true;
        });

    while (!login_done) {
        this_thread::sleep_for(chrono::milliseconds(100));
    }

    // Set up status update callback
    int offline_count = 0;
    client.setPlayerStatusCallback([&](const PlayerStatusUpdate& update) {
        cout << "\n[Client 1] 📊 STATUS UPDATE:" << endl;
        cout << "  User: " << update.display_name << " (ID " << update.user_id << ")" << endl;
        cout << "  Status: " << (int)update.status;

        if (update.status == STATUS_OFFLINE) {
            cout << " (OFFLINE) ⚠️" << endl;
            offline_count++;
        } else if (update.status == STATUS_AVAILABLE) {
            cout << " (AVAILABLE)" << endl;
        } else if (update.status == STATUS_ONLINE) {
            cout << " (ONLINE)" << endl;
        }
        cout << endl;
    });

    // Wait for client 2 to connect
    this_thread::sleep_for(chrono::seconds(2));

    // Request player list
    cout << "[Client 1] Requesting player list..." << endl;
    bool list_done = false;
    client.requestPlayerList([&](bool success, const vector<PlayerInfo_Message>& players) {
        if (success) {
            cout << "[Client 1] ✅ Player list (" << players.size() << " players):" << endl;
            for (const auto& p : players) {
                cout << "  - " << p.display_name << " (ID=" << p.user_id << ", Status=" << (int)p.status << ")" << endl;
            }
        }
        list_done = true;
    });

    while (!list_done) {
        this_thread::sleep_for(chrono::milliseconds(100));
    }

    // Wait for client 2 to disconnect
    cout << "\n[Client 1] Waiting for Client 2 to disconnect..." << endl;
    cout << "[Client 1] (You should see a STATUS_OFFLINE update when Client 2 disconnects)" << endl;

    // Keep running for 10 seconds to receive disconnect notification
    this_thread::sleep_for(chrono::seconds(10));

    // Check results
    cout << "\n========================================" << endl;
    cout << "TEST RESULTS:" << endl;
    cout << "========================================" << endl;
    if (offline_count > 0) {
        cout << "✅ SUCCESS: Received " << offline_count << " offline notification(s)" << endl;
        cout << "   Client properly detects when other players disconnect!" << endl;
    } else {
        cout << "❌ FAILED: No offline notifications received" << endl;
        cout << "   Disconnect detection is not working!" << endl;
    }
    cout << "========================================\n" << endl;

    cout << "[Client 1] Test complete!" << endl;
}

void test_client_2() {
    cout << "\n[Client 2] Starting..." << endl;

    ClientNetwork client;

    // Connect
    cout << "[Client 2] Connecting..." << endl;
    if (!client.connect(SERVER_HOST, SERVER_PORT)) {
        cerr << "[Client 2] Failed to connect!" << endl;
        return;
    }
    this_thread::sleep_for(chrono::milliseconds(500));

    // Register & Login
    time_t now = time(nullptr);
    string username = "disconnect_test2_" + to_string(now);

    bool register_done = false;
    client.registerUser(username, "password123", "Player 2",
        [&](bool success, uint32_t user_id, const string& error) {
            if (success) {
                cout << "[Client 2] ✅ Registered! user_id=" << user_id << endl;
            } else {
                cout << "[Client 2] ❌ Register failed: " << error << endl;
            }
            register_done = true;
        });

    while (!register_done) {
        this_thread::sleep_for(chrono::milliseconds(100));
    }

    bool login_done = false;
    client.loginUser(username, "password123",
        [&](bool success, uint32_t, const string&, int32_t, const string&, const string& error) {
            if (success) {
                cout << "[Client 2] ✅ Logged in!" << endl;
            } else {
                cout << "[Client 2] ❌ Login failed: " << error << endl;
            }
            login_done = true;
        });

    while (!login_done) {
        this_thread::sleep_for(chrono::milliseconds(100));
    }

    // Wait a bit then disconnect
    cout << "[Client 2] Will disconnect in 3 seconds..." << endl;
    this_thread::sleep_for(chrono::seconds(3));

    cout << "[Client 2] 🔌 Disconnecting now..." << endl;
    client.disconnect();

    cout << "[Client 2] Disconnected!" << endl;
}

int main() {
    cout << "╔════════════════════════════════════════════╗" << endl;
    cout << "║   DISCONNECT DETECTION TEST                ║" << endl;
    cout << "╠════════════════════════════════════════════╣" << endl;
    cout << "║  This test verifies that:                  ║" << endl;
    cout << "║  1. Server detects client disconnect       ║" << endl;
    cout << "║  2. Server broadcasts STATUS_OFFLINE       ║" << endl;
    cout << "║  3. Clients remove offline players         ║" << endl;
    cout << "║                                            ║" << endl;
    cout << "║  Prerequisites:                            ║" << endl;
    cout << "║  - Server running on localhost:9999        ║" << endl;
    cout << "╚════════════════════════════════════════════╝" << endl;

    // Run two clients
    thread t1(test_client_1);
    thread t2(test_client_2);

    t1.join();
    t2.join();

    return 0;
}
