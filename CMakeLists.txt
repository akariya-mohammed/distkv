cmake_minimum_required(VERSION 3.15)
project(DistKV VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler warnings
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Include directories
include_directories(${PROJECT_SOURCE_DIR}/include)

# Source files
set(SOURCES
    src/storage.cpp
    src/protocol.cpp
    src/server.cpp
    src/persistence.cpp
    src/replication.cpp
)

# Server executable
add_executable(distkv-server
    ${SOURCES}
    src/main.cpp
)

# Client library
add_library(distkv-client STATIC
    client/client.cpp
)

# Client executable (interactive CLI)
add_executable(distkv-cli
    client/cli.cpp
)
target_link_libraries(distkv-cli distkv-client)

# Platform-specific libraries
if(WIN32)
    target_link_libraries(distkv-server ws2_32)
    target_link_libraries(distkv-cli ws2_32)
else()
    target_link_libraries(distkv-server pthread)
    target_link_libraries(distkv-cli pthread)
endif()

# Tests (optional - uncomment when adding test framework)
# enable_testing()
# add_subdirectory(tests)
