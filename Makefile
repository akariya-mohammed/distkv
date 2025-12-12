# Simple Makefile for DistKV (alternative to CMake)
# For Windows: Use MinGW or WSL
# For Linux/macOS: Use directly

CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Iinclude -pthread
LDFLAGS = -pthread

# Windows-specific
ifeq ($(OS),Windows_NT)
    LDFLAGS += -lws2_32
    EXE_EXT = .exe
else
    EXE_EXT =
endif

# Source files
SERVER_SRCS = src/storage.cpp src/protocol.cpp src/server.cpp \
              src/persistence.cpp src/replication.cpp src/main.cpp

CLIENT_LIB_SRCS = client/client.cpp
CLI_SRCS = client/cli.cpp
TEST_SRCS = tests/test_storage.cpp
BENCH_SRCS = benchmarks/bench.cpp

# Object files
SERVER_OBJS = $(SERVER_SRCS:.cpp=.o)
CLIENT_LIB_OBJS = $(CLIENT_LIB_SRCS:.cpp=.o)
CLI_OBJS = $(CLI_SRCS:.cpp=.o)
TEST_OBJS = $(TEST_SRCS:.cpp=.o)
BENCH_OBJS = $(BENCH_SRCS:.cpp=.o)

# Core library objects (without main.cpp)
CORE_OBJS = src/storage.o src/protocol.o src/server.o \
            src/persistence.o src/replication.o

# Targets
SERVER = distkv-server$(EXE_EXT)
CLI = distkv-cli$(EXE_EXT)
TEST = test-storage$(EXE_EXT)
BENCH = bench$(EXE_EXT)

.PHONY: all clean test benchmark full

all: $(SERVER) $(CLI)

# Build everything including tests and benchmarks
full: all $(TEST) $(BENCH)

$(SERVER): $(SERVER_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(CLI): $(CLI_OBJS) $(CLIENT_LIB_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(TEST): $(TEST_OBJS) $(CORE_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(BENCH): $(BENCH_OBJS) $(CLIENT_LIB_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Run tests
test: $(TEST)
	./$(TEST)

# Run benchmarks (requires server to be running)
benchmark: $(BENCH)
	@echo "Make sure server is running: ./$(SERVER)"
	./$(BENCH)

clean:
	rm -f $(SERVER_OBJS) $(CLIENT_LIB_OBJS) $(CLI_OBJS) $(TEST_OBJS) $(BENCH_OBJS)
	rm -f $(CORE_OBJS) $(SERVER) $(CLI) $(TEST) $(BENCH)
	rm -rf build/

# Install (optional)
install: all
	mkdir -p bin
	cp $(SERVER) $(CLI) bin/

.PHONY: help
help:
	@echo "DistKV Makefile"
	@echo ""
	@echo "Targets:"
	@echo "  all        - Build server and client (default)"
	@echo "  full       - Build everything (server, client, tests, benchmarks)"
	@echo "  test       - Build and run unit tests"
	@echo "  benchmark  - Build and run benchmarks"
	@echo "  clean      - Remove build artifacts"
	@echo "  install    - Copy binaries to bin/"
	@echo "  help       - Show this help"
