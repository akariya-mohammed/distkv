# Quick Start Guide

## 5-Minute Setup

### 1. Build the Project

```bash
cd distkv
mkdir build && cd build
cmake ..
cmake --build .
```

### 2. Start the Server

```bash
# In terminal 1
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

### 3. Test with CLI Client

```bash
# In terminal 2
./distkv-cli
```

### 4. Try Some Commands

```
distkv> SET greeting "Hello DistKV!"
OK

distkv> GET greeting
"Hello DistKV!"

distkv> LPUSH todos "Write documentation"
(integer) 1

distkv> LPUSH todos "Add tests"
(integer) 2

distkv> LRANGE todos 0 -1
(array) 2 elements:
  "Add tests"
  "Write documentation"

distkv> SADD languages "C++"
(integer) 1

distkv> SADD languages "Python"
(integer) 1

distkv> SMEMBERS languages
(array) 2 elements:
  "C++"
  "Python"

distkv> KEYS
(array) 3 elements:
  "greeting"
  "todos"
  "languages"
```

## Next Steps

1. **Explore all commands** - Type `HELP` in the CLI
2. **Test persistence** - Restart server and see data restored
3. **Try TTL** - Set expiring keys with `EXPIRE`
4. **Build a client** - Use the C++ client library in your own code
5. **Benchmark** - Compare performance with Redis
6. **Extend** - Add new features and commands

## Common Issues

### Windows Build Issues

If you see Winsock errors:
```bash
cmake .. -G "Visual Studio 16 2019"
cmake --build . --config Release
```

### Linux Permission Issues

If port 6379 is in use:
```bash
./distkv-server --port 8000
./distkv-cli -p 8000
```

### Can't Connect

Make sure server is running and firewall allows connections:
```bash
# Check if server is listening
netstat -an | grep 6379

# Or on Windows
netstat -an | findstr 6379
```

## Performance Tips

1. **Use persistence wisely** - Snapshots are async but still impact performance
2. **Monitor memory** - All data is in-memory
3. **Connection pooling** - Reuse client connections in production
4. **Batch operations** - Send multiple commands efficiently

## Development Workflow

```bash
# Make changes to code
vim src/storage.cpp

# Rebuild
cd build
cmake --build .

# Test
./distkv-server
```

Enjoy building with DistKV!
