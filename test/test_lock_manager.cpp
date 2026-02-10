/**
 * @file test_lock_manager.cpp
 * @brief 鎖管理器單元測試
 */

#include "../src/kvengine/lock_manager.h"
#include <iostream>
#include <cassert>
#include <thread>
#include <chrono>

using namespace kvengine;

// 測試基本的共享鎖
void test_shared_lock() {
    std::cout << "Testing shared lock..." << std::endl;
    
    LockManager lock_mgr;
    
    // 兩個事務可以同時持有共享鎖
    assert(lock_mgr.lock_shared(1, "key1"));
    assert(lock_mgr.lock_shared(2, "key1"));
    
    // 釋放鎖
    assert(lock_mgr.unlock(1, "key1"));
    assert(lock_mgr.unlock(2, "key1"));
    
    std::cout << "  ✓ Shared lock test passed" << std::endl;
}

// 測試排他鎖
void test_exclusive_lock() {
    std::cout << "Testing exclusive lock..." << std::endl;
    
    LockManager lock_mgr;
    
    // 獲取排他鎖
    assert(lock_mgr.lock_exclusive(1, "key1"));
    
    // 另一個事務無法獲取任何鎖（非阻塞測試）
    assert(!lock_mgr.try_lock(2, "key1", LockMode::SHARED));
    assert(!lock_mgr.try_lock(2, "key1", LockMode::EXCLUSIVE));
    
    // 釋放鎖後可以獲取
    assert(lock_mgr.unlock(1, "key1"));
    assert(lock_mgr.try_lock(2, "key1", LockMode::EXCLUSIVE));
    
    assert(lock_mgr.unlock(2, "key1"));
    
    std::cout << "  ✓ Exclusive lock test passed" << std::endl;
}

// 測試鎖衝突
void test_lock_conflict() {
    std::cout << "Testing lock conflict..." << std::endl;
    
    LockManager lock_mgr;
    
    // 共享鎖與共享鎖兼容
    assert(lock_mgr.lock_shared(1, "key1"));
    assert(lock_mgr.try_lock(2, "key1", LockMode::SHARED));
    
    // 共享鎖與排他鎖衝突
    assert(!lock_mgr.try_lock(3, "key1", LockMode::EXCLUSIVE));
    
    // 釋放所有共享鎖
    assert(lock_mgr.unlock(1, "key1"));
    assert(lock_mgr.unlock(2, "key1"));
    
    // 現在可以獲取排他鎖
    assert(lock_mgr.try_lock(3, "key1", LockMode::EXCLUSIVE));
    
    // 排他鎖與任何鎖衝突
    assert(!lock_mgr.try_lock(4, "key1", LockMode::SHARED));
    assert(!lock_mgr.try_lock(4, "key1", LockMode::EXCLUSIVE));
    
    assert(lock_mgr.unlock(3, "key1"));
    
    std::cout << "  ✓ Lock conflict test passed" << std::endl;
}

// 測試 unlock_all
void test_unlock_all() {
    std::cout << "Testing unlock_all..." << std::endl;
    
    LockManager lock_mgr;
    
    // 事務獲取多個鎖
    assert(lock_mgr.lock_shared(1, "key1"));
    assert(lock_mgr.lock_exclusive(1, "key2"));
    assert(lock_mgr.lock_shared(1, "key3"));
    
    // 其他事務無法獲取這些鎖
    assert(!lock_mgr.try_lock(2, "key2", LockMode::SHARED));
    
    // 釋放所有鎖
    assert(lock_mgr.unlock_all(1));
    
    // 現在可以獲取
    assert(lock_mgr.try_lock(2, "key1", LockMode::EXCLUSIVE));
    assert(lock_mgr.try_lock(2, "key2", LockMode::EXCLUSIVE));
    assert(lock_mgr.try_lock(2, "key3", LockMode::EXCLUSIVE));
    
    assert(lock_mgr.unlock_all(2));
    
    std::cout << "  ✓ unlock_all test passed" << std::endl;
}

// 測試鎖升級
void test_lock_upgrade() {
    std::cout << "Testing lock upgrade..." << std::endl;
    
    LockManager lock_mgr;
    
    // 獲取共享鎖
    assert(lock_mgr.lock_shared(1, "key1"));
    
    // 嘗試升級到排他鎖（只有一個共享鎖時可以升級）
    assert(lock_mgr.try_lock(1, "key1", LockMode::EXCLUSIVE));
    
    // 清理
    assert(lock_mgr.unlock_all(1));
    
    // 多個共享鎖時無法升級
    assert(lock_mgr.lock_shared(1, "key2"));
    assert(lock_mgr.lock_shared(2, "key2"));
    
    // 事務 1 無法升級（因為事務 2 也持有共享鎖）
    assert(!lock_mgr.try_lock(1, "key2", LockMode::EXCLUSIVE));
    
    assert(lock_mgr.unlock_all(1));
    assert(lock_mgr.unlock_all(2));
    
    std::cout << "  ✓ Lock upgrade test passed" << std::endl;
}

// 測試並發鎖（使用線程）
void test_concurrent_locks() {
    std::cout << "Testing concurrent locks..." << std::endl;
    
    LockManager lock_mgr;
    bool thread1_done = false;
    bool thread2_done = false;
    
    // 線程 1：獲取排他鎖，持有一段時間
    std::thread t1([&]() {
        lock_mgr.lock_exclusive(1, "key1");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        lock_mgr.unlock(1, "key1");
        thread1_done = true;
    });
    
    // 線程 2：等待獲取排他鎖
    std::thread t2([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        lock_mgr.lock_exclusive(2, "key1");
        assert(thread1_done);  // 確保線程 1 已經釋放鎖
        lock_mgr.unlock(2, "key1");
        thread2_done = true;
    });
    
    t1.join();
    t2.join();
    
    assert(thread1_done && thread2_done);
    
    std::cout << "  ✓ Concurrent locks test passed" << std::endl;
}

// 主測試函數
int main() {
    std::cout << "=== Lock Manager Test Suite ===" << std::endl << std::endl;
    
    try {
        test_shared_lock();
        test_exclusive_lock();
        test_lock_conflict();
        test_unlock_all();
        test_lock_upgrade();
        test_concurrent_locks();
        
        std::cout << std::endl << "=== All lock manager tests passed! ===" << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Test failed with unknown exception" << std::endl;
        return 1;
    }
}
