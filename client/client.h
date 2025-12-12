#ifndef DISTKV_CLIENT_H
#define DISTKV_CLIENT_H

#include <string>
#include <vector>
#include <optional>

namespace distkv {

class Client {
public:
    Client();
    ~Client();

    // Connection management
    bool connect(const std::string& host, int port);
    void disconnect();
    bool is_connected() const { return connected_; }

    // Commands
    bool ping();
    bool set(const std::string& key, const std::string& value);
    std::optional<std::string> get(const std::string& key);
    bool del(const std::string& key);
    bool exists(const std::string& key);
    bool expire(const std::string& key, int seconds);
    int ttl(const std::string& key);
    std::vector<std::string> keys();
    size_t dbsize();

    // List commands
    int lpush(const std::string& key, const std::string& value);
    int rpush(const std::string& key, const std::string& value);
    std::optional<std::string> lpop(const std::string& key);
    std::optional<std::string> rpop(const std::string& key);
    std::vector<std::string> lrange(const std::string& key, int start, int stop);
    int llen(const std::string& key);

    // Set commands
    bool sadd(const std::string& key, const std::string& member);
    bool srem(const std::string& key, const std::string& member);
    bool sismember(const std::string& key, const std::string& member);
    std::vector<std::string> smembers(const std::string& key);
    int scard(const std::string& key);

    // Get last error
    std::string get_error() const { return last_error_; }

private:
    int socket_fd_;
    bool connected_;
    std::string last_error_;

    // Send command and receive response
    bool send_command(const std::string& cmd);
    std::string receive_response();

    // Parse response
    struct ParseResult {
        bool success;
        std::vector<std::string> data;
        std::string error;
    };
    ParseResult parse_response(const std::string& response);
};

} // namespace distkv

#endif // DISTKV_CLIENT_H
