#include "protocol.h"
#include <sstream>
#include <algorithm>
#include <cctype>

namespace distkv {

Request Protocol::parse_request(const std::string& input) {
    Request req;

    // Simple whitespace-based parsing for now
    // Format: "COMMAND arg1 arg2 arg3"
    std::istringstream iss(input);
    std::string cmd_str;

    if (!(iss >> cmd_str)) {
        req.command = CommandType::UNKNOWN;
        return req;
    }

    // Convert command to uppercase
    std::transform(cmd_str.begin(), cmd_str.end(), cmd_str.begin(),
                   [](unsigned char c) { return std::toupper(c); });

    req.command = string_to_command(cmd_str);

    // Parse arguments
    std::string arg;
    while (iss >> arg) {
        req.args.push_back(arg);
    }

    return req;
}

std::string Protocol::serialize_response(const Response& response) {
    std::ostringstream oss;

    switch (response.status) {
        case StatusCode::OK:
            if (response.data.empty()) {
                oss << "+OK\r\n";
            } else if (response.data.size() == 1) {
                // Single bulk string
                const auto& str = response.data[0];
                oss << "$" << str.length() << "\r\n" << str << "\r\n";
            } else {
                // Array of bulk strings
                oss << "*" << response.data.size() << "\r\n";
                for (const auto& item : response.data) {
                    oss << "$" << item.length() << "\r\n" << item << "\r\n";
                }
            }
            break;

        case StatusCode::NOT_FOUND:
            oss << "$-1\r\n";  // Null bulk string
            break;

        case StatusCode::ERROR:
            oss << "-ERR ";
            if (!response.data.empty()) {
                oss << response.data[0];
            } else {
                oss << "unknown error";
            }
            oss << "\r\n";
            break;

        case StatusCode::WRONG_TYPE:
            oss << "-WRONGTYPE Operation against a key holding the wrong kind of value\r\n";
            break;

        case StatusCode::INVALID_ARGS:
            oss << "-ERR wrong number of arguments\r\n";
            break;
    }

    return oss.str();
}

CommandType Protocol::string_to_command(const std::string& cmd) {
    if (cmd == "SET") return CommandType::SET;
    if (cmd == "GET") return CommandType::GET;
    if (cmd == "DEL") return CommandType::DEL;
    if (cmd == "EXISTS") return CommandType::EXISTS;
    if (cmd == "EXPIRE") return CommandType::EXPIRE;
    if (cmd == "TTL") return CommandType::TTL;
    if (cmd == "KEYS") return CommandType::KEYS;
    if (cmd == "DBSIZE") return CommandType::DBSIZE;
    if (cmd == "LPUSH") return CommandType::LPUSH;
    if (cmd == "RPUSH") return CommandType::RPUSH;
    if (cmd == "LPOP") return CommandType::LPOP;
    if (cmd == "RPOP") return CommandType::RPOP;
    if (cmd == "LRANGE") return CommandType::LRANGE;
    if (cmd == "LLEN") return CommandType::LLEN;
    if (cmd == "SADD") return CommandType::SADD;
    if (cmd == "SREM") return CommandType::SREM;
    if (cmd == "SISMEMBER") return CommandType::SISMEMBER;
    if (cmd == "SMEMBERS") return CommandType::SMEMBERS;
    if (cmd == "SCARD") return CommandType::SCARD;
    if (cmd == "PING") return CommandType::PING;
    if (cmd == "QUIT") return CommandType::QUIT;

    return CommandType::UNKNOWN;
}

std::string Protocol::command_to_string(CommandType cmd) {
    switch (cmd) {
        case CommandType::SET: return "SET";
        case CommandType::GET: return "GET";
        case CommandType::DEL: return "DEL";
        case CommandType::EXISTS: return "EXISTS";
        case CommandType::EXPIRE: return "EXPIRE";
        case CommandType::TTL: return "TTL";
        case CommandType::KEYS: return "KEYS";
        case CommandType::DBSIZE: return "DBSIZE";
        case CommandType::LPUSH: return "LPUSH";
        case CommandType::RPUSH: return "RPUSH";
        case CommandType::LPOP: return "LPOP";
        case CommandType::RPOP: return "RPOP";
        case CommandType::LRANGE: return "LRANGE";
        case CommandType::LLEN: return "LLEN";
        case CommandType::SADD: return "SADD";
        case CommandType::SREM: return "SREM";
        case CommandType::SISMEMBER: return "SISMEMBER";
        case CommandType::SMEMBERS: return "SMEMBERS";
        case CommandType::SCARD: return "SCARD";
        case CommandType::PING: return "PING";
        case CommandType::QUIT: return "QUIT";
        default: return "UNKNOWN";
    }
}

} // namespace distkv
