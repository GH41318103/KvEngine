cmake_minimum_required(VERSION 3.10)

# Basic usage example
add_executable(basic_usage basic_usage.cpp)
target_link_libraries(basic_usage kvengine)

# Batch operations example
add_executable(batch_operations batch_operations.cpp)
target_link_libraries(batch_operations kvengine)

# Set output directory
set_target_properties(basic_usage batch_operations
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples
)

message(STATUS "Examples configured:")
message(STATUS "  - basic_usage")
message(STATUS "  - batch_operations")
