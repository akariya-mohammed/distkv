# DistKV - Distributed Key-Value Store

A Redis-inspired distributed in-memory key-value store built from scratch in C++. This project demonstrates deep systems programming knowledge including network programming, concurrent data structures, persistence mechanisms, and distributed systems concepts.

## Features

### Core Functionality
- **In-Memory Storage** - Fast hash-based key-value storage with multiple data types
- **Multiple Data Types** - Strings, Lists, and Sets
- **Thread-Safe** - Concurrent access with reader-writer locks
- **TTL Support** - Automatic key expiration
- **Persistence** - Snapshot-based (RDB) persistence
- **Network Protocol** - Redis-compatible RESP protocol
- **Client Library** - Full-featured C++ client with CLI

### Supported Data Types

#### Strings
- `SET key value` - Store a string
- `GET key` - Retrieve a string
- `DEL key` - Delete a key

#### Lists
- `LPUSH key value` - Push to head
- `RPUSH key value` - Push to tail
- `LPOP key` - Pop from head
- `RPOP key` - Pop from tail
- `LRANGE key start stop` - Get range
- `LLEN key` - Get length

#### Sets
- `SADD key member` - Add member
- `SREM key member` - Remove member
- `SISMEMBER key member` - Check membership
- `SMEMBERS key` - Get all members
- `SCARD key` - Get cardinality

#### Generic
- `EXISTS key` - Check if key exists
- `EXPIRE key seconds` - Set expiration
- `TTL key` - Get time-to-live
- `KEYS` - List all keys
- `DBSIZE` - Database size

## Building the Project

### Prerequisites
- CMake 3.15 or higher
- C++17 compatible compiler (GCC, Clang, MSVC)
- Windows: Visual Studio 2019+ or MinGW
- Linux/macOS: GCC 7+ or Clang 5+

### Build Instructions

```bash
# Clone the repository
git clone https://github.com/yourusername/distkv.git
cd distkv

# Create build directory
mkdir build && cd build

# Configure and build
cmake ..
cmake --build .
```

On Windows with Visual Studio:
```bash
cmake ..
cmake --build . --config Release
```

## Usage

### Starting the Server

```bash
# Start with default settings (port 6379)
./distkv-server

# Custom port
./distkv-server --port 8000

# Custom snapshot file
./distkv-server --snapshot /path/to/dump.rdb

# Show help
./distkv-server --help
```

### Using the CLI Client

```bash
# Connect to localhost:6379
./distkv-cli

# Connect to custom host/port
./distkv-cli -h 192.168.1.100 -p 8000
```

### Example Session

```
$ ./distkv-cli
DistKV CLI - Connecting to 127.0.0.1:6379...
Connected! Type 'HELP' for available commands.

distkv> SET mykey "Hello World"
OK

distkv> GET mykey
"Hello World"

distkv> EXPIRE mykey 60
(integer) 1

distkv> TTL mykey
(integer) 58

distkv> LPUSH mylist item1
(integer) 1

distkv> LPUSH mylist item2
(integer) 2

distkv> LRANGE mylist 0 -1
(array) 2 elements:
  "item2"
  "item1"

distkv> SADD myset member1
(integer) 1

distkv> SADD myset member2
(integer) 1

distkv> SMEMBERS myset
(array) 2 elements:
  "member1"
  "member2"

distkv> KEYS
(array) 3 elements:
  "mykey"
  "mylist"
  "myset"

distkv> QUIT
Goodbye!
```

### Using the C++ Client Library

```cpp
#include "client.h"
#include <iostream>

int main() {
    distkv::Client client;

    // Connect to server
    if (!client.connect("127.0.0.1", 6379)) {
        std::cerr << "Connection failed: " << client.get_error() << "\n";
        return 1;
    }

    // Set a value
    client.set("name", "Mohammad");

    // Get a value
    auto value = client.get("name");
    if (value) {
        std::cout << "Name: " << *value << "\n";
    }

    // List operations
    client.lpush("tasks", "task1");
    client.lpush("tasks", "task2");
    auto tasks = client.lrange("tasks", 0, -1);

    // Set operations
    client.sadd("users", "alice");
    client.sadd("users", "bob");
    bool is_member = client.sismember("users", "alice");

    client.disconnect();
    return 0;
}
```

## Architecture

### System Components

```
┌─────────────────────────────────────────────┐
│         Client Applications                 │
└──────────────┬──────────────────────────────┘
               │ RESP Protocol (TCP)
┌──────────────▼──────────────────────────────┐
│         Network Layer (Server)              │
│  ┌──────────────────────────────────────┐   │
│  │   Multithreaded Connection Handler  │   │
│  └──────────────┬───────────────────────┘   │
└─────────────────┼───────────────────────────┘
                  │
┌─────────────────▼───────────────────────────┐
│         Command Processing Layer            │
│  ┌─────────────────────────────────────┐    │
│  │  Parser → Executor → Response       │    │
│  └─────────────────────────────────────┘    │
└─────────────────┬───────────────────────────┘
                  │
┌─────────────────▼───────────────────────────┐
│         Storage Engine (Thread-Safe)        │
│  ┌──────────────────┬──────────────────┐    │
│  │  Hash Table      │  Data Structures │    │
│  │  (RW Locks)      │  (String/List)   │    │
│  └──────────────────┴──────────────────┘    │
└─────────────────┬───────────────────────────┘
                  │
┌─────────────────▼───────────────────────────┐
│         Persistence Layer                   │
│  ┌──────────────┬───────────────────────┐   │
│  │  Snapshots   │  Future: AOF         │   │
│  │  (RDB)       │                       │   │
│  └──────────────┴───────────────────────┘   │
└─────────────────────────────────────────────┘
```

### Concurrency Model

- **Storage Engine**: Uses `std::shared_mutex` for reader-writer locks
- **Server**: Spawns a new thread per client connection
- **Thread Safety**: All operations are thread-safe with fine-grained locking

### Protocol

DistKV uses a simplified RESP (REdis Serialization Protocol):

**Request Format**: Simple text-based commands
```
SET mykey myvalue\n
GET mykey\n
```

**Response Format**: RESP-compatible
```
+OK\r\n                    # Simple string
$5\r\nHello\r\n           # Bulk string
*2\r\n$3\r\nfoo\r\n...    # Array
-ERR message\r\n          # Error
```

## Project Structure

```
distkv/
├── CMakeLists.txt          # Build configuration
├── README.md               # This file
├── include/                # Header files
│   ├── storage.h          # Storage engine interface
│   ├── server.h           # Server interface
│   ├── protocol.h         # Protocol parser/serializer
│   ├── persistence.h      # Persistence interface
│   └── replication.h      # Replication (future)
├── src/                    # Implementation files
│   ├── storage.cpp        # Core storage implementation
│   ├── server.cpp         # Network server
│   ├── protocol.cpp       # Protocol handling
│   ├── persistence.cpp    # Snapshot save/load
│   ├── replication.cpp    # Replication (placeholder)
│   └── main.cpp           # Server entry point
├── client/                 # Client library
│   ├── client.h           # Client interface
│   ├── client.cpp         # Client implementation
│   └── cli.cpp            # Interactive CLI
├── tests/                  # Unit tests (future)
├── benchmarks/             # Performance tests (future)
└── data/                   # Default data directory
```

## Future Enhancements

### Planned Features
- [ ] **Append-Only File (AOF)** - Write-ahead logging for durability
- [ ] **Master-Slave Replication** - Async replication with failover
- [ ] **Pub/Sub** - Message broadcasting
- [ ] **Transactions** - MULTI/EXEC support
- [ ] **Clustering** - Distributed hash table across nodes
- [ ] **Event Loop** - epoll/kqueue for better scalability
- [ ] **Benchmarking Suite** - Performance comparisons with Redis
- [ ] **Sorted Sets** - Ordered set data type
- [ ] **Lua Scripting** - Embedded scripting support

### Performance Optimizations
- [ ] Lock-free data structures
- [ ] Memory pooling
- [ ] Zero-copy networking
- [ ] Compression support

## Testing

```bash
# Build tests
cd build
cmake .. -DBUILD_TESTS=ON
cmake --build .

# Run tests
ctest
```

## Benchmarking

Compare performance with Redis using redis-benchmark:

```bash
# DistKV
redis-benchmark -p 6379 -t set,get -n 100000 -q

# Redis
redis-benchmark -p 6380 -t set,get -n 100000 -q
```

## Technical Highlights

### What This Project Demonstrates

1. **Systems Programming**
   - Low-level socket programming (POSIX + Winsock)
   - Thread synchronization and concurrency
   - Memory management with smart pointers

2. **Data Structures**
   - Hash tables with concurrent access
   - Thread-safe linked lists (via std::vector)
   - Efficient set implementations

3. **Network Programming**
   - TCP server implementation
   - Protocol design and parsing
   - Client-server communication

4. **Persistence**
   - Binary serialization/deserialization
   - Snapshot creation and recovery
   - Data integrity management

5. **Software Design**
   - Modular architecture
   - Clean interfaces and separation of concerns
   - Cross-platform compatibility (Windows/Linux/macOS)

## Contributing

Contributions are welcome! Areas for improvement:
- Additional data types (Sorted Sets, Hashes)
- Performance optimizations
- Better error handling
- More comprehensive tests
- Documentation improvements

## License

MIT License - feel free to use this project for learning and portfolio purposes.

## Author

**Mohammad Akariya**
- GitHub: [@akariya-mohammed](https://github.com/akariya-mohammed)
- LinkedIn: [Mohammad Akariya](https://www.linkedin.com/in/mohammad-akariya-185808273)
- Email: makree29@gmail.com

## Acknowledgments

Inspired by Redis and built as a learning project to demonstrate systems programming expertise.

---

**Note**: This is an educational project designed to showcase software engineering skills. For production use, consider mature solutions like Redis, Memcached, or other established key-value stores.
