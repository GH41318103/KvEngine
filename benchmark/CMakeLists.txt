cmake_minimum_required(VERSION 3.10)

# Benchmark executable
add_executable(kv_benchmark kv_benchmark.cpp)
target_link_libraries(kv_benchmark kvengine)

# Set output directory
set_target_properties(kv_benchmark
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/benchmark
)

message(STATUS "Benchmarks configured:")
message(STATUS "  - kv_benchmark")
