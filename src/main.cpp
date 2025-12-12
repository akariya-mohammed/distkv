#include "server.h"
#include "persistence.h"
#include <iostream>
#include <csignal>
#include <cstring>

using namespace distkv;

// Global server instance for signal handling
Server* g_server = nullptr;

void signal_handler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        std::cout << "\nShutting down server...\n";
        if (g_server) {
            g_server->stop();
        }
    }
}

int main(int argc, char* argv[]) {
    int port = 6379;  // Default Redis port
    std::string snapshot_file = "data/dump.rdb";

    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--port") == 0 && i + 1 < argc) {
            port = std::atoi(argv[i + 1]);
            ++i;
        } else if (std::strcmp(argv[i], "--snapshot") == 0 && i + 1 < argc) {
            snapshot_file = argv[i + 1];
            ++i;
        } else if (std::strcmp(argv[i], "--help") == 0) {
            std::cout << "DistKV - Distributed Key-Value Store\n\n";
            std::cout << "Usage: " << argv[0] << " [options]\n\n";
            std::cout << "Options:\n";
            std::cout << "  --port <port>         Port to listen on (default: 6379)\n";
            std::cout << "  --snapshot <file>     Snapshot file path (default: data/dump.rdb)\n";
            std::cout << "  --help                Show this help message\n";
            return 0;
        }
    }

    std::cout << R"(
    ____  _     _   __ ____   __
   / __ \(_)___/ |_/ //_/ /  / /
  / / / / / ___/ __/ ,< / /  / /
 / /_/ / (__  ) /_/ /| / /__/ /
/_____/_/____/\__/_/ |_\___/_/

Distributed Key-Value Store v1.0.0
)" << "\n";

    // Create server
    Server server(port, 4);
    g_server = &server;

    // Register signal handlers
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    // Try to load snapshot
    std::cout << "Attempting to load snapshot from " << snapshot_file << "...\n";
    if (Persistence::load_snapshot(*server.get_storage(), snapshot_file)) {
        std::cout << "Snapshot loaded successfully.\n";
    } else {
        std::cout << "No snapshot found or failed to load. Starting with empty database.\n";
    }

    // Start server (blocking)
    server.start();

    // Save snapshot before exiting
    std::cout << "Saving snapshot...\n";
    Persistence::save_snapshot(*server.get_storage(), snapshot_file);

    return 0;
}
