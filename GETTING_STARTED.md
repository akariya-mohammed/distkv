# Getting Started with DistKV

Welcome to DistKV! This guide will help you build, run, and explore the project in under 10 minutes.

## Prerequisites

- C++17 compiler (GCC 7+, Clang 5+, or MSVC 2019+)
- CMake 3.15+ or Make
- Git (for cloning)

## Quick Start (5 Minutes)

### Step 1: Clone and Build

```bash
# Clone the repository
git clone https://github.com/yourusername/distkv.git
cd distkv

# Build with CMake
mkdir build && cd build
cmake ..
cmake --build .

# OR build with Make
make
```

### Step 2: Start the Server

```bash
# From build directory (CMake)
./distkv-server

# OR from root directory (Make)
./distkv-server
```

You should see:
```
    ____  _     _   __ ____   __
   / __ \(_)___/ |_/ //_/ /  / /
  / / / / / ___/ __/ ,< / /  / /
 / /_/ / (__  ) /_/ /| / /__/ /
/_____/_/____/\__/_/ |_\___/_/

Distributed Key-Value Store v1.0.0

DistKV server starting on port 6379...
Ready to accept connections.
```

### Step 3: Connect with CLI

Open a new terminal:

```bash
# From build directory
./distkv-cli

# OR from root directory
./distkv-cli
```

### Step 4: Try Commands

```
distkv> SET greeting "Hello DistKV!"
OK

distkv> GET greeting
"Hello DistKV!"

distkv> LPUSH tasks "Learn DistKV"
(integer) 1

distkv> LPUSH tasks "Build something cool"
(integer) 2

distkv> LRANGE tasks 0 -1
(array) 2 elements:
  "Build something cool"
  "Learn DistKV"

distkv> QUIT
Goodbye!
```

Congratulations! You've successfully run DistKV! 🎉

---

## Detailed Build Instructions

### Linux / macOS

#### Using CMake (Recommended)
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .

# Binaries will be in build/
./distkv-server
./distkv-cli
```

#### Using Make
```bash
make
# Binaries will be in root directory
./distkv-server
./distkv-cli
```

### Windows

#### Visual Studio
```bash
mkdir build && cd build
cmake .. -G "Visual Studio 16 2019"
cmake --build . --config Release

# Binaries in build/Release/
.\Release\distkv-server.exe
.\Release\distkv-cli.exe
```

#### MinGW
```bash
mkdir build && cd build
cmake .. -G "MinGW Makefiles"
cmake --build .

.\distkv-server.exe
.\distkv-cli.exe
```

---

## Running Tests

### Build and Run Unit Tests

```bash
# With CMake
cd build
cmake .. -DBUILD_TESTS=ON
cmake --build .
ctest

# With Make
make test
```

Expected output:
```
=================================
Running DistKV Storage Tests
=================================

Testing string operations... ✓
Testing list operations... ✓
Testing set operations... ✓
Testing expiration... ✓
Testing concurrent access... ✓

=================================
All tests passed! ✓
=================================
```

---

## Running Benchmarks

### Performance Testing

```bash
# Terminal 1: Start server
./distkv-server

# Terminal 2: Run benchmarks
make benchmark
# OR
./bench
```

Expected output:
```
========================================
     DistKV Performance Benchmark
========================================

Connected to server at 127.0.0.1:6379

Benchmarking SET operations...
  Operations: 10000
  Duration: 650 ms
  Throughput: 15384.62 ops/sec
  Avg Latency: 0.065 ms

Benchmarking GET operations...
  Operations: 10000
  Duration: 520 ms
  Throughput: 19230.77 ops/sec
  Avg Latency: 0.052 ms

...
```

---

## Common Commands Reference

### String Operations
```bash
SET key value              # Store a string
GET key                    # Retrieve a string
DEL key                    # Delete a key
EXISTS key                 # Check if key exists
EXPIRE key seconds         # Set expiration time
TTL key                    # Get time to live
```

### List Operations
```bash
LPUSH key value           # Push to head
RPUSH key value           # Push to tail
LPOP key                  # Pop from head
RPOP key                  # Pop from tail
LRANGE key start stop     # Get range
LLEN key                  # Get length
```

### Set Operations
```bash
SADD key member           # Add member
SREM key member           # Remove member
SISMEMBER key member      # Check membership
SMEMBERS key              # Get all members
SCARD key                 # Get cardinality
```

### Server Operations
```bash
KEYS                      # List all keys
DBSIZE                    # Get database size
PING                      # Test connection
QUIT                      # Exit
```

---

## Configuration Options

### Server Options

```bash
./distkv-server --help

Options:
  --port <port>         Port to listen on (default: 6379)
  --snapshot <file>     Snapshot file path (default: data/dump.rdb)
  --help                Show help message
```

### Client Options

```bash
./distkv-cli -h <host> -p <port>

Options:
  -h <host>            Server hostname (default: 127.0.0.1)
  -p <port>            Server port (default: 6379)
```

---

## Using the C++ Client Library

### Basic Example

Create `example.cpp`:
```cpp
#include "client/client.h"
#include <iostream>

int main() {
    distkv::Client client;

    // Connect
    if (!client.connect("127.0.0.1", 6379)) {
        std::cerr << "Connection failed\n";
        return 1;
    }

    // String operations
    client.set("name", "Mohammad");
    auto name = client.get("name");
    if (name) {
        std::cout << "Name: " << *name << "\n";
    }

    // List operations
    client.lpush("languages", "C++");
    client.lpush("languages", "Python");
    client.lpush("languages", "JavaScript");

    auto langs = client.lrange("languages", 0, -1);
    std::cout << "Languages:\n";
    for (const auto& lang : langs) {
        std::cout << "  - " << lang << "\n";
    }

    // Set operations
    client.sadd("skills", "Systems Programming");
    client.sadd("skills", "Networking");

    bool has_skill = client.sismember("skills", "Systems Programming");
    std::cout << "Has Systems Programming: "
              << (has_skill ? "Yes" : "No") << "\n";

    client.disconnect();
    return 0;
}
```

### Build and Run

```bash
# Link against client library
g++ -std=c++17 -Iinclude example.cpp client/client.cpp -o example -pthread

# On Windows, add -lws2_32
g++ -std=c++17 -Iinclude example.cpp client/client.cpp -o example -lws2_32

# Run (make sure server is running)
./example
```

---

## Troubleshooting

### Build Errors

**Error: cmake not found**
```bash
# Ubuntu/Debian
sudo apt-get install cmake

# macOS
brew install cmake

# Windows: Download from cmake.org
```

**Error: C++17 required**
```bash
# Update compiler
sudo apt-get install g++-9  # Ubuntu
```

### Runtime Errors

**Error: Port already in use**
```bash
# Use different port
./distkv-server --port 8000
./distkv-cli -p 8000
```

**Error: Connection refused**
```bash
# Make sure server is running
ps aux | grep distkv-server

# Check if port is listening
netstat -an | grep 6379  # Linux/Mac
netstat -an | findstr 6379  # Windows
```

**Error: Snapshot loading failed**
```bash
# Delete corrupted snapshot
rm data/dump.rdb

# Restart server
./distkv-server
```

---

## Next Steps

### Learn More
1. **Read the [README](README.md)** - Full feature documentation
2. **Check [DEVELOPMENT.md](DEVELOPMENT.md)** - Add new features
3. **Review [PROJECT_SUMMARY.md](PROJECT_SUMMARY.md)** - Technical deep dive

### Try These Challenges
1. **Persistence Test**: Set data, restart server, verify data persists
2. **TTL Test**: Set key with `EXPIRE`, wait, verify it disappears
3. **Concurrent Test**: Run multiple CLI clients simultaneously
4. **Performance Test**: Run benchmarks with different workloads

### Extend the Project
1. Add a new command (e.g., APPEND for strings)
2. Implement Hash data type
3. Add AOF (Append-Only File) persistence
4. Implement Pub/Sub messaging
5. Create a web dashboard

---

## Resources

### Documentation
- [README.md](README.md) - Project overview
- [QUICKSTART.md](QUICKSTART.md) - 5-minute guide
- [DEVELOPMENT.md](DEVELOPMENT.md) - Developer guide
- [PROJECT_SUMMARY.md](PROJECT_SUMMARY.md) - Technical summary

### External Resources
- [Redis Commands](https://redis.io/commands) - Command reference
- [RESP Protocol](https://redis.io/docs/reference/protocol-spec/) - Protocol spec
- [C++ Reference](https://en.cppreference.com/) - C++ documentation

---

## Getting Help

### Issues
If you encounter problems:
1. Check the [Troubleshooting](#troubleshooting) section
2. Review build output for errors
3. Check if server is running (`ps aux | grep distkv`)
4. Verify port is not in use (`netstat -an | grep 6379`)

### Community
- **GitHub Issues**: Report bugs or request features
- **Discussions**: Ask questions and share ideas

---

## Quick Reference Card

```
┌─────────────────────────────────────────────┐
│         DistKV Quick Reference              │
├─────────────────────────────────────────────┤
│ Build:                                      │
│   cmake .. && cmake --build .               │
│   OR make                                   │
│                                             │
│ Run Server:                                 │
│   ./distkv-server                           │
│                                             │
│ Run Client:                                 │
│   ./distkv-cli                              │
│                                             │
│ Common Commands:                            │
│   SET key value     - Store string          │
│   GET key           - Retrieve string       │
│   LPUSH key value   - Push to list          │
│   SADD key member   - Add to set            │
│   KEYS              - List all keys         │
│   QUIT              - Exit                  │
│                                             │
│ Test:                                       │
│   make test                                 │
│                                             │
│ Benchmark:                                  │
│   make benchmark                            │
└─────────────────────────────────────────────┘
```

Happy coding! 🚀
