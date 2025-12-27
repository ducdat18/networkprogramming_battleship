#include <iostream>
#include <csignal>
#include <cstdlib>
#include <memory>
#include <thread>
#include <chrono>
#include "server.h"
#include "config.h"

// Global server instance for signal handling
std::unique_ptr<Server> g_server;

// Signal handler for graceful shutdown
void signalHandler(int signal) {
    std::cout << "\n[SIGNAL] Received signal " << signal << std::endl;
    if (g_server) {
        std::cout << "[SERVER] Shutting down gracefully..." << std::endl;
        g_server->stop();
    }
    exit(0);
}

int main(int argc, char* argv[]) {
    // Parse command line arguments
    int port = SERVER_PORT;  // Use config.h default
    if (argc > 1) {
        port = std::atoi(argv[1]);
        if (port <= 0 || port > 65535) {
            std::cerr << "Invalid port number: " << argv[1] << std::endl;
            std::cerr << "Usage: " << argv[0] << " [port]" << std::endl;
            return 1;
        }
    }

    // Setup signal handlers
    std::signal(SIGINT, signalHandler);   // Ctrl+C
    std::signal(SIGTERM, signalHandler);  // kill command

    // Banner
    std::cout << "╔════════════════════════════════════╗" << std::endl;
    std::cout << "║  ⚓ BATTLESHIP GAME SERVER ⚓      ║" << std::endl;
    std::cout << "║  Version: 1.0                     ║" << std::endl;
    std::cout << "║  Port: " << port << "                         ║" << std::endl;
    std::cout << "╚════════════════════════════════════╝" << std::endl;
    std::cout << std::endl;

    // Create and start server
    g_server = std::make_unique<Server>(port);

    if (!g_server->start()) {
        std::cerr << "[ERROR] Failed to start server" << std::endl;
        return 1;
    }

    std::cout << "[SERVER] Server started successfully!" << std::endl;
    std::cout << "[SERVER] Listening on port " << port << std::endl;
    std::cout << "[SERVER] Press Ctrl+C to stop" << std::endl;
    std::cout << std::endl;

    // Main loop (server runs in background threads)
    while (g_server->isRunning()) {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        // Print statistics every 30 seconds
        static int counter = 0;
        if (++counter % 30 == 0) {
            std::cout << "[STATS] Connected clients: " << g_server->getConnectedClients()
                      << " | Active matches: " << g_server->getActiveMatches() << std::endl;
        }
    }

    return 0;
}
