# Development Guide

## Project Overview

DistKV is a Redis-inspired distributed key-value store demonstrating advanced systems programming concepts in C++.

## Development Setup

### Required Tools
- C++17 compiler (GCC 7+, Clang 5+, MSVC 2019+)
- CMake 3.15+ (recommended) or Make
- Git
- Optional: GDB/LLDB for debugging, Valgrind for memory checks

### Building from Source

#### Using CMake (Recommended)
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build .
```

#### Using Make
```bash
make
```

#### Windows with Visual Studio
```bash
cmake .. -G "Visual Studio 16 2019"
cmake --build . --config Debug
```

## Code Structure

### Core Components

#### 1. Storage Engine ([storage.h](include/storage.h), [storage.cpp](src/storage.cpp))
- Thread-safe hash table using `std::shared_mutex`
- Supports STRING, LIST, and SET data types
- TTL and expiration handling
- Memory-efficient value storage with `std::shared_ptr`

**Key Classes:**
```cpp
class Storage {
    // Thread-safe operations
    bool set(const std::string& key, const std::string& value);
    std::optional<std::string> get(const std::string& key);

    // Data structure
    std::unordered_map<std::string, std::shared_ptr<Value>> data_;
    std::shared_mutex mutex_;
};
```

#### 2. Network Server ([server.h](include/server.h), [server.cpp](src/server.cpp))
- TCP socket server (cross-platform: Winsock/POSIX)
- Thread-per-connection model
- Command parsing and execution
- Response serialization

**Architecture:**
```cpp
class Server {
    void start();                              // Main server loop
    void handle_client(int client_fd);        // Per-connection handler
    Response execute_command(const Request&);  // Command executor
};
```

#### 3. Protocol ([protocol.h](include/protocol.h), [protocol.cpp](src/protocol.cpp))
- RESP-compatible protocol
- Request parsing: `COMMAND arg1 arg2...`
- Response formatting: `+OK\r\n`, `$5\r\nvalue\r\n`, etc.

#### 4. Persistence ([persistence.h](include/persistence.h), [persistence.cpp](src/persistence.cpp))
- Binary snapshot format (RDB-style)
- Serialization/deserialization of all data types
- Atomic save/load operations

#### 5. Client Library ([client.h](client/client.h), [client.cpp](client/client.cpp))
- C++ client with type-safe API
- Socket communication
- Response parsing

## Adding New Features

### Example 1: Add a New String Command (APPEND)

**Step 1: Define Command in Protocol**
```cpp
// include/protocol.h
enum class CommandType : uint8_t {
    // ...
    APPEND = 0x03,  // Add new command
};
```

**Step 2: Implement Storage Logic**
```cpp
// include/storage.h
class Storage {
    int append(const std::string& key, const std::string& value);
};

// src/storage.cpp
int Storage::append(const std::string& key, const std::string& value) {
    std::unique_lock<std::shared_mutex> lock(mutex_);

    auto it = data_.find(key);
    if (it == data_.end() || it->second->type != ValueType::STRING) {
        // Create new string if doesn't exist
        set(key, value);
        return value.length();
    }

    auto str_ptr = std::static_pointer_cast<std::string>(it->second->data);
    *str_ptr += value;
    return str_ptr->length();
}
```

**Step 3: Add Server Handler**
```cpp
// src/server.cpp
Response Server::execute_command(const Request& req) {
    switch (req.command) {
        // ...
        case CommandType::APPEND: {
            if (req.args.size() != 2) {
                return Response(StatusCode::INVALID_ARGS);
            }
            int len = storage_->append(req.args[0], req.args[1]);
            return Response(StatusCode::OK, std::to_string(len));
        }
    }
}
```

**Step 4: Update Protocol Parser**
```cpp
// src/protocol.cpp
CommandType Protocol::string_to_command(const std::string& cmd) {
    // ...
    if (cmd == "APPEND") return CommandType::APPEND;
}
```

**Step 5: Add Client Method**
```cpp
// client/client.h
class Client {
    int append(const std::string& key, const std::string& value);
};

// client/client.cpp
int Client::append(const std::string& key, const std::string& value) {
    std::string cmd = "APPEND " + key + " " + value;
    if (!send_command(cmd)) return 0;
    std::string resp = receive_response();
    auto result = parse_response(resp);
    if (result.success && !result.data.empty()) {
        return std::stoi(result.data[0]);
    }
    return 0;
}
```

### Example 2: Add a New Data Type (Hash)

**Step 1: Define Value Type**
```cpp
// include/storage.h
enum class ValueType {
    STRING,
    LIST,
    SET,
    HASH  // New type
};
```

**Step 2: Create Storage Methods**
```cpp
// include/storage.h
class Storage {
    bool hset(const std::string& key, const std::string& field,
              const std::string& value);
    std::optional<std::string> hget(const std::string& key,
                                     const std::string& field);
    // ... more hash operations
};

// src/storage.cpp
bool Storage::hset(const std::string& key, const std::string& field,
                    const std::string& value) {
    std::unique_lock<std::shared_mutex> lock(mutex_);

    auto val = get_or_create(key, ValueType::HASH);
    if (!val || val->type != ValueType::HASH) {
        return false;
    }

    auto hash_ptr = std::static_pointer_cast<
        std::unordered_map<std::string, std::string>>(val->data);
    (*hash_ptr)[field] = value;
    return true;
}
```

**Step 3: Update Value Creation**
```cpp
// src/storage.cpp - in get_or_create()
case ValueType::HASH:
    val->data = std::make_shared<
        std::unordered_map<std::string, std::string>>();
    break;
```

**Step 4: Add Serialization Support**
```cpp
// src/persistence.cpp
void Persistence::serialize_value(std::ostream& os,
                                   const std::shared_ptr<Value>& value) {
    // ...
    case ValueType::HASH: {
        auto hash_ptr = std::static_pointer_cast<
            std::unordered_map<std::string, std::string>>(value->data);
        size_t count = hash_ptr->size();
        os.write(reinterpret_cast<const char*>(&count), sizeof(count));
        for (const auto& [field, val] : *hash_ptr) {
            // Write field
            size_t field_len = field.length();
            os.write(reinterpret_cast<const char*>(&field_len), sizeof(field_len));
            os.write(field.c_str(), field_len);
            // Write value
            size_t val_len = val.length();
            os.write(reinterpret_cast<const char*>(&val_len), sizeof(val_len));
            os.write(val.c_str(), val_len);
        }
        break;
    }
}
```

## Debugging

### Using GDB
```bash
gdb ./distkv-server
(gdb) break Storage::set
(gdb) run
(gdb) bt
(gdb) print key
```

### Memory Leak Detection (Linux)
```bash
valgrind --leak-check=full ./distkv-server
```

### Thread Sanitizer
```bash
cmake .. -DCMAKE_CXX_FLAGS="-fsanitize=thread"
cmake --build .
./distkv-server
```

## Testing Strategy

### Unit Tests
```cpp
// tests/test_storage.cpp
#include "storage.h"
#include <cassert>

void test_set_get() {
    distkv::Storage storage;

    storage.set("key1", "value1");
    auto val = storage.get("key1");

    assert(val.has_value());
    assert(*val == "value1");
}

int main() {
    test_set_get();
    // More tests...
    return 0;
}
```

### Integration Tests
```bash
# Start server
./distkv-server &

# Run tests
./distkv-cli <<EOF
SET test1 value1
GET test1
DEL test1
QUIT
EOF

# Stop server
killall distkv-server
```

## Performance Optimization Tips

### 1. Reduce Lock Contention
- Use finer-grained locks per key/bucket
- Implement lock-free data structures for read-heavy workloads

### 2. Memory Optimization
- Use memory pooling for small allocations
- Compress large values
- Implement LRU eviction policy

### 3. Network Optimization
- Pipelining support
- Batch command processing
- TCP_NODELAY for low latency

### 4. Persistence Optimization
- Background save (fork process)
- Incremental snapshots
- Efficient binary encoding

## Code Style

### Naming Conventions
- Classes: `PascalCase`
- Functions: `snake_case`
- Member variables: `snake_case_` (trailing underscore)
- Constants: `UPPER_SNAKE_CASE`

### Comments
```cpp
// Brief description of what the function does
bool Storage::set(const std::string& key, const std::string& value) {
    // Implementation details if non-obvious
}
```

### Error Handling
```cpp
// Use std::optional for nullable returns
std::optional<std::string> get(const std::string& key);

// Use bool for success/failure
bool del(const std::string& key);

// Throw exceptions for unrecoverable errors (sparingly)
```

## Benchmarking

### Using redis-benchmark
```bash
# Test SET performance
redis-benchmark -p 6379 -t set -n 100000 -q

# Test GET performance
redis-benchmark -p 6379 -t get -n 100000 -q

# Mixed workload
redis-benchmark -p 6379 -t set,get,lpush,lpop -n 50000 -q
```

### Custom Benchmark
```cpp
// benchmarks/bench.cpp
#include "client.h"
#include <chrono>

int main() {
    distkv::Client client;
    client.connect("127.0.0.1", 6379);

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 100000; ++i) {
        client.set("key" + std::to_string(i), "value");
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end - start).count();

    std::cout << "Ops/sec: " << (100000.0 / duration * 1000) << "\n";

    return 0;
}
```

## Contributing Guidelines

1. **Fork the repository**
2. **Create a feature branch**: `git checkout -b feature/amazing-feature`
3. **Make changes and test thoroughly**
4. **Commit with clear messages**: `git commit -m "Add HSET command for hash support"`
5. **Push to branch**: `git push origin feature/amazing-feature`
6. **Create Pull Request**

## Resources

- [Redis Protocol Specification](https://redis.io/docs/reference/protocol-spec/)
- [C++ Concurrency in Action](https://www.manning.com/books/c-plus-plus-concurrency-in-action)
- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/)
- [The Linux Programming Interface](https://man7.org/tlpi/)

## Common Issues

### Issue: Port already in use
**Solution**: Change port or kill process using it
```bash
# Linux/macOS
lsof -i :6379
kill -9 <PID>

# Windows
netstat -ano | findstr :6379
taskkill /PID <PID> /F
```

### Issue: Segmentation fault
**Solution**: Use debugger and sanitizers
```bash
gdb ./distkv-server
(gdb) run
(gdb) bt
```

### Issue: Race conditions
**Solution**: Thread sanitizer
```bash
cmake .. -DCMAKE_CXX_FLAGS="-fsanitize=thread"
```

Happy coding!
