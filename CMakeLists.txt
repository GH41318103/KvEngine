cmake_minimum_required(VERSION 3.10)
project(KvEngine VERSION 0.1.0 LANGUAGES CXX)

# C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler flags
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(-O3)
    endif()
endif()

# Options
option(BUILD_EXAMPLES "Build examples" ON)
option(BUILD_TESTS "Build tests" ON)
option(BUILD_BENCHMARKS "Build benchmarks" ON)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/src)

# Source files
set(KVENGINE_SOURCES
    src/kvengine/kv_engine.cpp
    src/kvengine/storage_engine.cpp
    src/kvengine/hash_index.cpp
    src/kvengine/memory_manager.cpp
    src/kvengine/iterator.cpp
    src/kvengine/wal.cpp
    src/kvengine/lock_manager.cpp
    src/kvengine/transaction.cpp
    src/kvengine/transaction_manager.cpp
    src/kvengine/checkpoint_manager.cpp
    src/kvengine/recovery_manager.cpp
    src/kvengine/network/socket.cpp
    src/kvengine/network/tcp_server.cpp
    src/kvengine/network/resp_parser.cpp
    src/kvengine/network/resp_builder.cpp
    src/kvengine/network/command_dispatcher.cpp
    src/kvengine/network/kv_server.cpp
    src/kvengine/storage/page_manager.cpp
    src/kvengine/storage/buffer_pool_manager.cpp
)

# Create library
add_library(kvengine STATIC ${KVENGINE_SOURCES})
target_include_directories(kvengine PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Platform-specific settings
if(WIN32)
    target_compile_definitions(kvengine PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_link_libraries(kvengine PUBLIC ws2_32)
endif()

# Examples
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Server Executable
add_executable(kv_server src/server_main.cpp)
target_link_libraries(kv_server PRIVATE kvengine)

# Client Executable
add_executable(kv_client src/client_main.cpp)
target_link_libraries(kv_client PRIVATE kvengine)
if(WIN32)
    target_link_libraries(kv_server PRIVATE ws2_32)
    target_link_libraries(kv_client PRIVATE ws2_32)
endif()

# Tests
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()

# Benchmarks
if(BUILD_BENCHMARKS)
    add_subdirectory(benchmark)
endif()

# Install rules
install(TARGETS kvengine
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/kvengine
    DESTINATION include
)

# Print configuration
message(STATUS "KvEngine Configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build examples: ${BUILD_EXAMPLES}")
message(STATUS "  Build tests: ${BUILD_TESTS}")
message(STATUS "  Build benchmarks: ${BUILD_BENCHMARKS}")
