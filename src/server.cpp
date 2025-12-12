#include "server.h"
#include <iostream>
#include <cstring>

// Platform-specific includes
#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    typedef int socklen_t;
    #define CLOSE_SOCKET closesocket
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #define CLOSE_SOCKET close
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
#endif

namespace distkv {

Server::Server(int port, int num_threads)
    : port_(port),
      num_threads_(num_threads),
      running_(false),
      storage_(std::make_unique<Storage>()),
      listen_fd_(INVALID_SOCKET) {

#ifdef _WIN32
    // Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed\n";
    }
#endif
}

Server::~Server() {
    stop();

#ifdef _WIN32
    WSACleanup();
#endif
}

bool Server::init_socket() {
    // Create socket
    listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd_ == INVALID_SOCKET) {
        std::cerr << "Failed to create socket\n";
        return false;
    }

    // Set socket options
    int opt = 1;
#ifdef _WIN32
    setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
#else
    setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif

    // Bind socket
    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port_);

    if (bind(listen_fd_, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        std::cerr << "Failed to bind socket to port " << port_ << "\n";
        CLOSE_SOCKET(listen_fd_);
        return false;
    }

    // Listen
    if (listen(listen_fd_, 10) == SOCKET_ERROR) {
        std::cerr << "Failed to listen on socket\n";
        CLOSE_SOCKET(listen_fd_);
        return false;
    }

    return true;
}

void Server::start() {
    if (running_) {
        std::cerr << "Server already running\n";
        return;
    }

    if (!init_socket()) {
        return;
    }

    running_ = true;
    std::cout << "DistKV server starting on port " << port_ << "...\n";
    std::cout << "Ready to accept connections.\n";

    // Accept connections in main thread (simple approach for now)
    while (running_) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        int client_fd = accept(listen_fd_, (struct sockaddr*)&client_addr, &client_len);

        if (client_fd == INVALID_SOCKET) {
            if (running_) {
                std::cerr << "Failed to accept connection\n";
            }
            continue;
        }

        // Handle client in separate thread
        std::thread([this, client_fd]() {
            handle_client(client_fd);
        }).detach();
    }
}

void Server::stop() {
    if (!running_) {
        return;
    }

    running_ = false;

    if (listen_fd_ != INVALID_SOCKET) {
        CLOSE_SOCKET(listen_fd_);
        listen_fd_ = INVALID_SOCKET;
    }

    std::cout << "Server stopped.\n";
}

void Server::handle_client(int client_fd) {
    char buffer[4096];
    std::string accumulated;

    while (running_) {
#ifdef _WIN32
        int bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
#else
        ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
#endif

        if (bytes_read <= 0) {
            break;  // Connection closed or error
        }

        buffer[bytes_read] = '\0';
        accumulated += buffer;

        // Process complete commands (ending with \n)
        size_t pos;
        while ((pos = accumulated.find('\n')) != std::string::npos) {
            std::string line = accumulated.substr(0, pos);
            accumulated = accumulated.substr(pos + 1);

            // Remove carriage return if present
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }

            if (line.empty()) {
                continue;
            }

            // Parse and execute command
            Request req = Protocol::parse_request(line);
            Response resp = execute_command(req);

            // Send response
            std::string response_str = Protocol::serialize_response(resp);
            send(client_fd, response_str.c_str(), response_str.length(), 0);

            // Check for QUIT command
            if (req.command == CommandType::QUIT) {
                CLOSE_SOCKET(client_fd);
                return;
            }
        }
    }

    CLOSE_SOCKET(client_fd);
}

Response Server::execute_command(const Request& req) {
    switch (req.command) {
        case CommandType::PING:
            return Response(StatusCode::OK, "PONG");

        case CommandType::SET: {
            if (req.args.size() != 2) {
                return Response(StatusCode::INVALID_ARGS);
            }
            storage_->set(req.args[0], req.args[1]);
            return Response(StatusCode::OK);
        }

        case CommandType::GET: {
            if (req.args.size() != 1) {
                return Response(StatusCode::INVALID_ARGS);
            }
            auto val = storage_->get(req.args[0]);
            if (val) {
                return Response(StatusCode::OK, *val);
            }
            return Response(StatusCode::NOT_FOUND);
        }

        case CommandType::DEL: {
            if (req.args.size() != 1) {
                return Response(StatusCode::INVALID_ARGS);
            }
            bool deleted = storage_->del(req.args[0]);
            return Response(StatusCode::OK, deleted ? "1" : "0");
        }

        case CommandType::EXISTS: {
            if (req.args.size() != 1) {
                return Response(StatusCode::INVALID_ARGS);
            }
            bool exists = storage_->exists(req.args[0]);
            return Response(StatusCode::OK, exists ? "1" : "0");
        }

        case CommandType::EXPIRE: {
            if (req.args.size() != 2) {
                return Response(StatusCode::INVALID_ARGS);
            }
            try {
                int seconds = std::stoi(req.args[1]);
                bool success = storage_->expire(req.args[0], seconds);
                return Response(StatusCode::OK, success ? "1" : "0");
            } catch (...) {
                return Response(StatusCode::ERROR, "invalid timeout value");
            }
        }

        case CommandType::TTL: {
            if (req.args.size() != 1) {
                return Response(StatusCode::INVALID_ARGS);
            }
            int ttl = storage_->ttl(req.args[0]);
            return Response(StatusCode::OK, std::to_string(ttl));
        }

        case CommandType::KEYS: {
            auto keys = storage_->keys();
            return Response(StatusCode::OK, keys);
        }

        case CommandType::DBSIZE: {
            size_t size = storage_->dbsize();
            return Response(StatusCode::OK, std::to_string(size));
        }

        case CommandType::LPUSH: {
            if (req.args.size() != 2) {
                return Response(StatusCode::INVALID_ARGS);
            }
            bool success = storage_->lpush(req.args[0], req.args[1]);
            if (!success) {
                return Response(StatusCode::WRONG_TYPE);
            }
            int len = storage_->llen(req.args[0]);
            return Response(StatusCode::OK, std::to_string(len));
        }

        case CommandType::RPUSH: {
            if (req.args.size() != 2) {
                return Response(StatusCode::INVALID_ARGS);
            }
            bool success = storage_->rpush(req.args[0], req.args[1]);
            if (!success) {
                return Response(StatusCode::WRONG_TYPE);
            }
            int len = storage_->llen(req.args[0]);
            return Response(StatusCode::OK, std::to_string(len));
        }

        case CommandType::LPOP: {
            if (req.args.size() != 1) {
                return Response(StatusCode::INVALID_ARGS);
            }
            auto val = storage_->lpop(req.args[0]);
            if (val) {
                return Response(StatusCode::OK, *val);
            }
            return Response(StatusCode::NOT_FOUND);
        }

        case CommandType::RPOP: {
            if (req.args.size() != 1) {
                return Response(StatusCode::INVALID_ARGS);
            }
            auto val = storage_->rpop(req.args[0]);
            if (val) {
                return Response(StatusCode::OK, *val);
            }
            return Response(StatusCode::NOT_FOUND);
        }

        case CommandType::LRANGE: {
            if (req.args.size() != 3) {
                return Response(StatusCode::INVALID_ARGS);
            }
            try {
                int start = std::stoi(req.args[1]);
                int stop = std::stoi(req.args[2]);
                auto list = storage_->lrange(req.args[0], start, stop);
                if (list) {
                    return Response(StatusCode::OK, *list);
                }
                return Response(StatusCode::NOT_FOUND);
            } catch (...) {
                return Response(StatusCode::ERROR, "invalid index");
            }
        }

        case CommandType::LLEN: {
            if (req.args.size() != 1) {
                return Response(StatusCode::INVALID_ARGS);
            }
            int len = storage_->llen(req.args[0]);
            return Response(StatusCode::OK, std::to_string(len));
        }

        case CommandType::SADD: {
            if (req.args.size() != 2) {
                return Response(StatusCode::INVALID_ARGS);
            }
            bool added = storage_->sadd(req.args[0], req.args[1]);
            return Response(StatusCode::OK, added ? "1" : "0");
        }

        case CommandType::SREM: {
            if (req.args.size() != 2) {
                return Response(StatusCode::INVALID_ARGS);
            }
            bool removed = storage_->srem(req.args[0], req.args[1]);
            return Response(StatusCode::OK, removed ? "1" : "0");
        }

        case CommandType::SISMEMBER: {
            if (req.args.size() != 2) {
                return Response(StatusCode::INVALID_ARGS);
            }
            bool is_member = storage_->sismember(req.args[0], req.args[1]);
            return Response(StatusCode::OK, is_member ? "1" : "0");
        }

        case CommandType::SMEMBERS: {
            if (req.args.size() != 1) {
                return Response(StatusCode::INVALID_ARGS);
            }
            auto members = storage_->smembers(req.args[0]);
            if (members) {
                std::vector<std::string> vec(members->begin(), members->end());
                return Response(StatusCode::OK, vec);
            }
            return Response(StatusCode::NOT_FOUND);
        }

        case CommandType::SCARD: {
            if (req.args.size() != 1) {
                return Response(StatusCode::INVALID_ARGS);
            }
            int card = storage_->scard(req.args[0]);
            return Response(StatusCode::OK, std::to_string(card));
        }

        case CommandType::QUIT:
            return Response(StatusCode::OK, "Goodbye");

        default:
            return Response(StatusCode::ERROR, "unknown command");
    }
}

} // namespace distkv
