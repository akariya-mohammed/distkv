#ifndef DISTKV_PROTOCOL_H
#define DISTKV_PROTOCOL_H

#include <string>
#include <vector>
#include <cstdint>

namespace distkv {

// Command types
enum class CommandType : uint8_t {
    // String commands
    SET = 0x01,
    GET = 0x02,

    // Generic commands
    DEL = 0x10,
    EXISTS = 0x11,
    EXPIRE = 0x12,
    TTL = 0x13,
    KEYS = 0x14,
    DBSIZE = 0x15,

    // List commands
    LPUSH = 0x20,
    RPUSH = 0x21,
    LPOP = 0x22,
    RPOP = 0x23,
    LRANGE = 0x24,
    LLEN = 0x25,

    // Set commands
    SADD = 0x30,
    SREM = 0x31,
    SISMEMBER = 0x32,
    SMEMBERS = 0x33,
    SCARD = 0x34,

    // Server commands
    PING = 0xF0,
    QUIT = 0xF1,

    UNKNOWN = 0xFF
};

// Response status codes
enum class StatusCode : uint8_t {
    OK = 0x00,
    ERROR = 0x01,
    NOT_FOUND = 0x02,
    WRONG_TYPE = 0x03,
    INVALID_ARGS = 0x04
};

// Request structure
struct Request {
    CommandType command;
    std::vector<std::string> args;

    Request() : command(CommandType::UNKNOWN) {}
    Request(CommandType cmd, const std::vector<std::string>& a)
        : command(cmd), args(a) {}
};

// Response structure
struct Response {
    StatusCode status;
    std::vector<std::string> data;

    Response() : status(StatusCode::ERROR) {}
    Response(StatusCode s) : status(s) {}
    Response(StatusCode s, const std::string& d) : status(s), data({d}) {}
    Response(StatusCode s, const std::vector<std::string>& d) : status(s), data(d) {}
};

// Protocol handler
class Protocol {
public:
    // Parse text-based protocol (Redis-like RESP)
    static Request parse_request(const std::string& input);

    // Serialize response to text format
    static std::string serialize_response(const Response& response);

    // Helper to convert command string to CommandType
    static CommandType string_to_command(const std::string& cmd);

    // Helper to convert CommandType to string
    static std::string command_to_string(CommandType cmd);
};

} // namespace distkv

#endif // DISTKV_PROTOCOL_H
