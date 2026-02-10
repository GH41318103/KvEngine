cmake_minimum_required(VERSION 3.10)

# Test executables
add_executable(test_kv_engine test_kv_engine.cpp)
target_link_libraries(test_kv_engine kvengine)

add_executable(test_wal test_wal.cpp)
target_link_libraries(test_wal kvengine)

add_executable(test_lock_manager test_lock_manager.cpp)
target_link_libraries(test_lock_manager kvengine)

add_executable(test_transaction test_transaction.cpp)
target_link_libraries(test_transaction kvengine)

add_executable(test_recovery test_recovery.cpp)
target_link_libraries(test_recovery kvengine)

add_executable(test_socket test_socket.cpp)
target_link_libraries(test_socket kvengine)

add_executable(test_tcp_server test_tcp_server.cpp)
target_link_libraries(test_tcp_server kvengine)

add_executable(test_resp test_resp.cpp)
target_link_libraries(test_resp kvengine)

add_executable(test_kv_server test_kv_server.cpp)
target_link_libraries(test_kv_server kvengine)

# Set output directory
set_target_properties(test_kv_engine test_wal test_lock_manager test_transaction test_recovery test_socket test_tcp_server test_resp test_kv_server
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/test
)

# Register tests
add_test(NAME KvEngineTest COMMAND test_kv_engine)
add_test(NAME WALTest COMMAND test_wal)
add_test(NAME LockManagerTest COMMAND test_lock_manager)
add_test(NAME TransactionTest COMMAND test_transaction)
add_test(NAME RecoveryTest COMMAND test_recovery)
add_test(NAME SocketTest COMMAND test_socket)
add_test(NAME TcpServerTest COMMAND test_tcp_server)
add_test(NAME RespTest COMMAND test_resp)
add_test(NAME KvServerTest COMMAND test_kv_server)

message(STATUS "Tests configured:")
message(STATUS "  - test_kv_engine")
message(STATUS "  - test_socket")
message(STATUS "  - test_tcp_server")
message(STATUS "  - test_resp")
message(STATUS "  - test_kv_server")
message(STATUS "  - test_kv_server")

# Page Manager Test
add_executable(test_page_manager test_page_manager.cpp)
target_link_libraries(test_page_manager kvengine)
add_test(NAME PageManagerTest COMMAND test_page_manager)
message(STATUS "  - test_page_manager")
message(STATUS "  - test_page_manager")

# Buffer Pool Test
add_executable(test_buffer_pool test_buffer_pool.cpp)
target_link_libraries(test_buffer_pool kvengine)
add_test(NAME BufferPoolTest COMMAND test_buffer_pool)
message(STATUS "  - test_buffer_pool")
message(STATUS "  - test_buffer_pool")

# B+ Tree Page Test
add_executable(test_b_plus_tree_page test_b_plus_tree_page.cpp)
target_link_libraries(test_b_plus_tree_page kvengine)
add_test(NAME BPlusTreePageTest COMMAND test_b_plus_tree_page)
message(STATUS "  - test_b_plus_tree_page")

# B+ Tree Test
add_executable(test_b_plus_tree test_b_plus_tree.cpp)
target_link_libraries(test_b_plus_tree kvengine)
add_test(NAME BPlusTreeTest COMMAND test_b_plus_tree)
message(STATUS "  - test_b_plus_tree")
