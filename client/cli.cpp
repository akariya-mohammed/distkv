#include "client.h"
#include <iostream>
#include <sstream>
#include <vector>

void print_help() {
    std::cout << "\nAvailable Commands:\n";
    std::cout << "  String commands:\n";
    std::cout << "    SET key value       - Set a string value\n";
    std::cout << "    GET key             - Get a string value\n";
    std::cout << "  \n";
    std::cout << "  Generic commands:\n";
    std::cout << "    DEL key             - Delete a key\n";
    std::cout << "    EXISTS key          - Check if key exists\n";
    std::cout << "    EXPIRE key seconds  - Set key expiration\n";
    std::cout << "    TTL key             - Get key time-to-live\n";
    std::cout << "    KEYS                - List all keys\n";
    std::cout << "    DBSIZE              - Get database size\n";
    std::cout << "  \n";
    std::cout << "  List commands:\n";
    std::cout << "    LPUSH key value     - Push to list head\n";
    std::cout << "    RPUSH key value     - Push to list tail\n";
    std::cout << "    LPOP key            - Pop from list head\n";
    std::cout << "    RPOP key            - Pop from list tail\n";
    std::cout << "    LRANGE key start stop - Get list range\n";
    std::cout << "    LLEN key            - Get list length\n";
    std::cout << "  \n";
    std::cout << "  Set commands:\n";
    std::cout << "    SADD key member     - Add to set\n";
    std::cout << "    SREM key member     - Remove from set\n";
    std::cout << "    SISMEMBER key member - Check set membership\n";
    std::cout << "    SMEMBERS key        - Get all set members\n";
    std::cout << "    SCARD key           - Get set cardinality\n";
    std::cout << "  \n";
    std::cout << "  Other:\n";
    std::cout << "    PING                - Test connection\n";
    std::cout << "    HELP                - Show this help\n";
    std::cout << "    QUIT                - Exit client\n";
    std::cout << "\n";
}

std::vector<std::string> parse_command(const std::string& input) {
    std::vector<std::string> tokens;
    std::istringstream iss(input);
    std::string token;

    while (iss >> token) {
        tokens.push_back(token);
    }

    return tokens;
}

int main(int argc, char* argv[]) {
    std::string host = "127.0.0.1";
    int port = 6379;

    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "-h" && i + 1 < argc) {
            host = argv[i + 1];
            ++i;
        } else if (std::string(argv[i]) == "-p" && i + 1 < argc) {
            port = std::atoi(argv[i + 1]);
            ++i;
        }
    }

    std::cout << "DistKV CLI - Connecting to " << host << ":" << port << "...\n";

    distkv::Client client;
    if (!client.connect(host, port)) {
        std::cerr << "Failed to connect: " << client.get_error() << "\n";
        return 1;
    }

    std::cout << "Connected! Type 'HELP' for available commands.\n\n";

    std::string line;
    while (true) {
        std::cout << "distkv> ";
        if (!std::getline(std::cin, line)) {
            break;
        }

        if (line.empty()) {
            continue;
        }

        auto tokens = parse_command(line);
        if (tokens.empty()) {
            continue;
        }

        std::string cmd = tokens[0];

        // Convert to uppercase
        for (char& c : cmd) {
            c = std::toupper(c);
        }

        if (cmd == "QUIT" || cmd == "EXIT") {
            std::cout << "Goodbye!\n";
            break;
        }

        if (cmd == "HELP") {
            print_help();
            continue;
        }

        if (cmd == "PING") {
            if (client.ping()) {
                std::cout << "PONG\n";
            } else {
                std::cout << "Error: " << client.get_error() << "\n";
            }
        } else if (cmd == "SET" && tokens.size() >= 3) {
            if (client.set(tokens[1], tokens[2])) {
                std::cout << "OK\n";
            } else {
                std::cout << "Error: " << client.get_error() << "\n";
            }
        } else if (cmd == "GET" && tokens.size() >= 2) {
            auto val = client.get(tokens[1]);
            if (val) {
                std::cout << "\"" << *val << "\"\n";
            } else {
                std::cout << "(nil)\n";
            }
        } else if (cmd == "DEL" && tokens.size() >= 2) {
            if (client.del(tokens[1])) {
                std::cout << "(integer) 1\n";
            } else {
                std::cout << "(integer) 0\n";
            }
        } else if (cmd == "EXISTS" && tokens.size() >= 2) {
            if (client.exists(tokens[1])) {
                std::cout << "(integer) 1\n";
            } else {
                std::cout << "(integer) 0\n";
            }
        } else if (cmd == "EXPIRE" && tokens.size() >= 3) {
            int seconds = std::atoi(tokens[2].c_str());
            if (client.expire(tokens[1], seconds)) {
                std::cout << "(integer) 1\n";
            } else {
                std::cout << "(integer) 0\n";
            }
        } else if (cmd == "TTL" && tokens.size() >= 2) {
            int ttl = client.ttl(tokens[1]);
            std::cout << "(integer) " << ttl << "\n";
        } else if (cmd == "KEYS") {
            auto keys = client.keys();
            std::cout << "(array) " << keys.size() << " elements:\n";
            for (const auto& key : keys) {
                std::cout << "  \"" << key << "\"\n";
            }
        } else if (cmd == "DBSIZE") {
            std::cout << "(integer) " << client.dbsize() << "\n";
        } else if (cmd == "LPUSH" && tokens.size() >= 3) {
            int len = client.lpush(tokens[1], tokens[2]);
            std::cout << "(integer) " << len << "\n";
        } else if (cmd == "RPUSH" && tokens.size() >= 3) {
            int len = client.rpush(tokens[1], tokens[2]);
            std::cout << "(integer) " << len << "\n";
        } else if (cmd == "LPOP" && tokens.size() >= 2) {
            auto val = client.lpop(tokens[1]);
            if (val) {
                std::cout << "\"" << *val << "\"\n";
            } else {
                std::cout << "(nil)\n";
            }
        } else if (cmd == "RPOP" && tokens.size() >= 2) {
            auto val = client.rpop(tokens[1]);
            if (val) {
                std::cout << "\"" << *val << "\"\n";
            } else {
                std::cout << "(nil)\n";
            }
        } else if (cmd == "LRANGE" && tokens.size() >= 4) {
            int start = std::atoi(tokens[2].c_str());
            int stop = std::atoi(tokens[3].c_str());
            auto list = client.lrange(tokens[1], start, stop);
            std::cout << "(array) " << list.size() << " elements:\n";
            for (const auto& item : list) {
                std::cout << "  \"" << item << "\"\n";
            }
        } else if (cmd == "LLEN" && tokens.size() >= 2) {
            std::cout << "(integer) " << client.llen(tokens[1]) << "\n";
        } else if (cmd == "SADD" && tokens.size() >= 3) {
            if (client.sadd(tokens[1], tokens[2])) {
                std::cout << "(integer) 1\n";
            } else {
                std::cout << "(integer) 0\n";
            }
        } else if (cmd == "SREM" && tokens.size() >= 3) {
            if (client.srem(tokens[1], tokens[2])) {
                std::cout << "(integer) 1\n";
            } else {
                std::cout << "(integer) 0\n";
            }
        } else if (cmd == "SISMEMBER" && tokens.size() >= 3) {
            if (client.sismember(tokens[1], tokens[2])) {
                std::cout << "(integer) 1\n";
            } else {
                std::cout << "(integer) 0\n";
            }
        } else if (cmd == "SMEMBERS" && tokens.size() >= 2) {
            auto members = client.smembers(tokens[1]);
            std::cout << "(array) " << members.size() << " elements:\n";
            for (const auto& member : members) {
                std::cout << "  \"" << member << "\"\n";
            }
        } else if (cmd == "SCARD" && tokens.size() >= 2) {
            std::cout << "(integer) " << client.scard(tokens[1]) << "\n";
        } else {
            std::cout << "Error: Unknown command or wrong number of arguments. Type HELP for help.\n";
        }
    }

    client.disconnect();
    return 0;
}
