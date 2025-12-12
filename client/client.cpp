#include "client.h"
#include <iostream>
#include <sstream>
#include <cstring>

// Platform-specific includes
#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    #define CLOSE_SOCKET closesocket
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #define CLOSE_SOCKET close
    #define INVALID_SOCKET -1
#endif

namespace distkv {

Client::Client() : socket_fd_(INVALID_SOCKET), connected_(false) {
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
}

Client::~Client() {
    disconnect();

#ifdef _WIN32
    WSACleanup();
#endif
}

bool Client::connect(const std::string& host, int port) {
    if (connected_) {
        disconnect();
    }

    // Create socket
    socket_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd_ == INVALID_SOCKET) {
        last_error_ = "Failed to create socket";
        return false;
    }

    // Setup address
    struct sockaddr_in server_addr;
    std::memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    // Convert IP address
    if (inet_pton(AF_INET, host.c_str(), &server_addr.sin_addr) <= 0) {
        last_error_ = "Invalid address";
        CLOSE_SOCKET(socket_fd_);
        socket_fd_ = INVALID_SOCKET;
        return false;
    }

    // Connect
    if (::connect(socket_fd_, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        last_error_ = "Connection failed";
        CLOSE_SOCKET(socket_fd_);
        socket_fd_ = INVALID_SOCKET;
        return false;
    }

    connected_ = true;
    return true;
}

void Client::disconnect() {
    if (socket_fd_ != INVALID_SOCKET) {
        CLOSE_SOCKET(socket_fd_);
        socket_fd_ = INVALID_SOCKET;
    }
    connected_ = false;
}

bool Client::send_command(const std::string& cmd) {
    if (!connected_) {
        last_error_ = "Not connected";
        return false;
    }

    std::string msg = cmd + "\n";
    ssize_t sent = send(socket_fd_, msg.c_str(), msg.length(), 0);

    if (sent <= 0) {
        last_error_ = "Failed to send command";
        disconnect();
        return false;
    }

    return true;
}

std::string Client::receive_response() {
    if (!connected_) {
        return "";
    }

    char buffer[4096];
    std::string response;

    while (true) {
        ssize_t received = recv(socket_fd_, buffer, sizeof(buffer) - 1, 0);

        if (received <= 0) {
            last_error_ = "Connection closed";
            disconnect();
            return "";
        }

        buffer[received] = '\0';
        response += buffer;

        // Check if we have a complete response (ends with \r\n)
        if (response.size() >= 2 && response.substr(response.size() - 2) == "\r\n") {
            break;
        }
    }

    return response;
}

Client::ParseResult Client::parse_response(const std::string& response) {
    ParseResult result;
    result.success = false;

    if (response.empty()) {
        result.error = "Empty response";
        return result;
    }

    char type = response[0];

    switch (type) {
        case '+': {  // Simple string
            result.success = true;
            result.data.push_back(response.substr(1, response.size() - 3));  // Remove +, \r\n
            break;
        }

        case '-': {  // Error
            result.success = false;
            result.error = response.substr(1, response.size() - 3);
            break;
        }

        case '$': {  // Bulk string
            if (response.substr(1, 2) == "-1") {
                // Null bulk string (key not found)
                result.success = false;
                result.error = "Not found";
            } else {
                // Find the content after the first \r\n
                size_t pos = response.find("\r\n");
                if (pos != std::string::npos) {
                    std::string content = response.substr(pos + 2);
                    content = content.substr(0, content.size() - 2);  // Remove trailing \r\n
                    result.success = true;
                    result.data.push_back(content);
                }
            }
            break;
        }

        case '*': {  // Array
            result.success = true;
            // Simple array parsing - split by \r\n and extract bulk strings
            std::istringstream iss(response.substr(1));
            int count;
            iss >> count;

            // For now, simple implementation
            // TODO: Proper RESP array parsing
            break;
        }

        default:
            result.error = "Unknown response type";
            break;
    }

    return result;
}

// ============= Command Implementations =============

bool Client::ping() {
    if (!send_command("PING")) return false;
    std::string resp = receive_response();
    auto result = parse_response(resp);
    return result.success;
}

bool Client::set(const std::string& key, const std::string& value) {
    std::string cmd = "SET " + key + " " + value;
    if (!send_command(cmd)) return false;
    std::string resp = receive_response();
    auto result = parse_response(resp);
    return result.success;
}

std::optional<std::string> Client::get(const std::string& key) {
    std::string cmd = "GET " + key;
    if (!send_command(cmd)) return std::nullopt;
    std::string resp = receive_response();
    auto result = parse_response(resp);
    if (result.success && !result.data.empty()) {
        return result.data[0];
    }
    return std::nullopt;
}

bool Client::del(const std::string& key) {
    std::string cmd = "DEL " + key;
    if (!send_command(cmd)) return false;
    std::string resp = receive_response();
    auto result = parse_response(resp);
    return result.success && !result.data.empty() && result.data[0] == "1";
}

bool Client::exists(const std::string& key) {
    std::string cmd = "EXISTS " + key;
    if (!send_command(cmd)) return false;
    std::string resp = receive_response();
    auto result = parse_response(resp);
    return result.success && !result.data.empty() && result.data[0] == "1";
}

bool Client::expire(const std::string& key, int seconds) {
    std::string cmd = "EXPIRE " + key + " " + std::to_string(seconds);
    if (!send_command(cmd)) return false;
    std::string resp = receive_response();
    auto result = parse_response(resp);
    return result.success && !result.data.empty() && result.data[0] == "1";
}

int Client::ttl(const std::string& key) {
    std::string cmd = "TTL " + key;
    if (!send_command(cmd)) return -2;
    std::string resp = receive_response();
    auto result = parse_response(resp);
    if (result.success && !result.data.empty()) {
        return std::stoi(result.data[0]);
    }
    return -2;
}

std::vector<std::string> Client::keys() {
    if (!send_command("KEYS")) return {};
    std::string resp = receive_response();
    auto result = parse_response(resp);
    return result.data;
}

size_t Client::dbsize() {
    if (!send_command("DBSIZE")) return 0;
    std::string resp = receive_response();
    auto result = parse_response(resp);
    if (result.success && !result.data.empty()) {
        return std::stoull(result.data[0]);
    }
    return 0;
}

int Client::lpush(const std::string& key, const std::string& value) {
    std::string cmd = "LPUSH " + key + " " + value;
    if (!send_command(cmd)) return 0;
    std::string resp = receive_response();
    auto result = parse_response(resp);
    if (result.success && !result.data.empty()) {
        return std::stoi(result.data[0]);
    }
    return 0;
}

int Client::rpush(const std::string& key, const std::string& value) {
    std::string cmd = "RPUSH " + key + " " + value;
    if (!send_command(cmd)) return 0;
    std::string resp = receive_response();
    auto result = parse_response(resp);
    if (result.success && !result.data.empty()) {
        return std::stoi(result.data[0]);
    }
    return 0;
}

std::optional<std::string> Client::lpop(const std::string& key) {
    std::string cmd = "LPOP " + key;
    if (!send_command(cmd)) return std::nullopt;
    std::string resp = receive_response();
    auto result = parse_response(resp);
    if (result.success && !result.data.empty()) {
        return result.data[0];
    }
    return std::nullopt;
}

std::optional<std::string> Client::rpop(const std::string& key) {
    std::string cmd = "RPOP " + key;
    if (!send_command(cmd)) return std::nullopt;
    std::string resp = receive_response();
    auto result = parse_response(resp);
    if (result.success && !result.data.empty()) {
        return result.data[0];
    }
    return std::nullopt;
}

std::vector<std::string> Client::lrange(const std::string& key, int start, int stop) {
    std::string cmd = "LRANGE " + key + " " + std::to_string(start) + " " + std::to_string(stop);
    if (!send_command(cmd)) return {};
    std::string resp = receive_response();
    auto result = parse_response(resp);
    return result.data;
}

int Client::llen(const std::string& key) {
    std::string cmd = "LLEN " + key;
    if (!send_command(cmd)) return 0;
    std::string resp = receive_response();
    auto result = parse_response(resp);
    if (result.success && !result.data.empty()) {
        return std::stoi(result.data[0]);
    }
    return 0;
}

bool Client::sadd(const std::string& key, const std::string& member) {
    std::string cmd = "SADD " + key + " " + member;
    if (!send_command(cmd)) return false;
    std::string resp = receive_response();
    auto result = parse_response(resp);
    return result.success && !result.data.empty() && result.data[0] == "1";
}

bool Client::srem(const std::string& key, const std::string& member) {
    std::string cmd = "SREM " + key + " " + member;
    if (!send_command(cmd)) return false;
    std::string resp = receive_response();
    auto result = parse_response(resp);
    return result.success && !result.data.empty() && result.data[0] == "1";
}

bool Client::sismember(const std::string& key, const std::string& member) {
    std::string cmd = "SISMEMBER " + key + " " + member;
    if (!send_command(cmd)) return false;
    std::string resp = receive_response();
    auto result = parse_response(resp);
    return result.success && !result.data.empty() && result.data[0] == "1";
}

std::vector<std::string> Client::smembers(const std::string& key) {
    std::string cmd = "SMEMBERS " + key;
    if (!send_command(cmd)) return {};
    std::string resp = receive_response();
    auto result = parse_response(resp);
    return result.data;
}

int Client::scard(const std::string& key) {
    std::string cmd = "SCARD " + key;
    if (!send_command(cmd)) return 0;
    std::string resp = receive_response();
    auto result = parse_response(resp);
    if (result.success && !result.data.empty()) {
        return std::stoi(result.data[0]);
    }
    return 0;
}

} // namespace distkv
