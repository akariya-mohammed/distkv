#ifndef DISTKV_SERVER_H
#define DISTKV_SERVER_H

#include "storage.h"
#include "protocol.h"
#include <memory>
#include <atomic>
#include <thread>
#include <vector>

namespace distkv {

class Server {
public:
    Server(int port, int num_threads = 4);
    ~Server();

    // Start the server (blocking)
    void start();

    // Stop the server
    void stop();

    // Get storage instance (for testing)
    Storage* get_storage() { return storage_.get(); }

private:
    int port_;
    int num_threads_;
    std::atomic<bool> running_;
    std::unique_ptr<Storage> storage_;
    std::vector<std::thread> worker_threads_;

    // Socket descriptor
    int listen_fd_;

    // Initialize socket
    bool init_socket();

    // Handle single client connection
    void handle_client(int client_fd);

    // Execute a command and return response
    Response execute_command(const Request& req);

    // Worker thread function
    void worker_thread();
};

} // namespace distkv

#endif // DISTKV_SERVER_H
