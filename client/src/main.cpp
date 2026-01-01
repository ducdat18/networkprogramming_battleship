#include "ui_manager.h"
#include "game_state.h"
#include <iostream>
#include <signal.h>
#include <stdlib.h>

void segfault_handler(int sig) {
    std::cerr << "\nERROR: SEGMENTATION FAULT (Signal " << sig << ")" << std::endl;
    std::cerr << "   This usually means:" << std::endl;
    std::cerr << "   1. X server is not running (install VcXsrv, Xming, or X410)" << std::endl;
    std::cerr << "   2. DISPLAY variable is set but X server is not accessible" << std::endl;
    std::cerr << "   3. Graphics drivers or GTK libraries are incompatible" << std::endl;
    std::cerr << "\n   For WSL2 users:" << std::endl;
    std::cerr << "   - Install VcXsrv on Windows" << std::endl;
    std::cerr << "   - Run it with \"Disable access control\" checked" << std::endl;
    std::cerr << "   - Set DISPLAY environment variable correctly" << std::endl;
    exit(1);
}

int main(int argc, char* argv[]) {
    // Register segfault handler for better error messages
    signal(SIGSEGV, segfault_handler);

    std::cout << "==================================" << std::endl;
    std::cout << "  BATTLESHIP ONLINE - CLIENT  " << std::endl;
    std::cout << "==================================" << std::endl;
    std::cout << "Version: 1.0" << std::endl;
    std::cout << "Platform: Linux/Ubuntu" << std::endl;
    std::cout << "Graphics: GTK+ 3.0" << std::endl;
    std::cout << "==================================" << std::endl << std::endl;

    try {
        // Create UI Manager
        UIManager ui_manager;

        // Initialize GTK and create window
        ui_manager.initialize(argc, argv);

        // Create game boards
        Board* player_board = new Board();
        Board* opponent_board = new Board();

        // Don't auto-place ships on player board - let user place them in ship placement screen
        // For testing: place ships on opponent board so we can test shooting mechanics
        opponent_board->randomPlacement();

        ui_manager.setPlayerBoard(player_board);
        ui_manager.setOpponentBoard(opponent_board);

        // Set up callbacks
        ui_manager.setOnCellClicked([](int row, int col) {
            std::cout << "Cell clicked: " << (char)('A' + row) << (col + 1) << std::endl;
        });

        ui_manager.setOnChatSend([](const std::string& message) {
            std::cout << "Chat: " << message << std::endl;
        });

        ui_manager.setOnResign([&ui_manager]() {
            std::cout << "[GAME] Player clicked resign button!" << std::endl;
            ui_manager.resignMatch();
        });

        ui_manager.setOnDrawOffer([]() {
            std::cout << "Draw offered!" << std::endl;
        });

        std::cout << "OK: UI initialized successfully!" << std::endl;
        std::cout << "Starting game..." << std::endl << std::endl;

        // Run GTK main loop
        ui_manager.run();

        // Cleanup
        delete player_board;
        delete opponent_board;

    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
        return 1;
    }

    std::cout << std::endl << "Thanks for playing!" << std::endl;
    return 0;
}
