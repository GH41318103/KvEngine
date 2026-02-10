/**
 * @file test_recovery.cpp
 * @brief 恢復管理器單元測試
 */

#include "../src/kvengine/recovery_manager.h"
#include "../src/kvengine/storage_engine.h"
#include "../src/kvengine/transaction_manager.h"
#include <iostream>
#include <cassert>
#include <vector>

using namespace kvengine;

// 測試簡單恢復 (Redo)
void test_recovery_redo() {
    std::cout << "Testing recovery redo..." << std::endl;
    
    std::string data_dir = "./test_recovery_redo";
    
    // 1. 模擬崩潰前的操作
    {
        StorageEngine storage(data_dir);
        if (!storage.initialize()) { std::cerr << "Storage init failed" << std::endl; abort(); }
        
        WAL wal(data_dir);
        if (!wal.initialize()) { std::cerr << "WAL init failed" << std::endl; abort(); }
        
        LockManager lock_mgr;
        TransactionManager txn_mgr(&wal, &lock_mgr, &storage);
        
        Transaction* txn = txn_mgr.begin();
        txn_mgr.put(txn, "key1", "val1");
        txn_mgr.commit(txn);
        delete txn;
        
        // 這裡我們不顯式調用 StorageEngine 的析構或保存，
        // 但由於目前的 StorageEngine 實現可能不會持久化到磁盤（內存 map），
        // 為了測試 Redo，我們必須假設 StorageEngine 在崩潰後是空的或舊的。
        // 在這個測試中，我們重新創建 StorageEngine，它是空的。
    }
    
    // 2. 模擬恢復
    {
        // 重新初始化全新的 StorageEngine (模擬數據丟失/崩潰重啟)
        StorageEngine storage(data_dir);
        if (!storage.initialize()) abort();
        
        // WAL 應該讀取之前寫入的日誌
        WAL wal(data_dir);
        if (!wal.initialize()) abort();
        
        RecoveryManager recovery(&wal, &storage);
        if (!recovery.recover()) abort();
        
        // 驗證 key1 是否被恢復
        std::string val;
        if (!storage.get("key1", val)) {
            std::cerr << "Key1 not found after recovery" << std::endl;
            abort();
        }
        if (val != "val1") {
            std::cerr << "Value mismatch: " << val << std::endl;
            abort();
        }
    }
    
    std::cout << "  ✓ Recovery redo test passed" << std::endl;
}

// 測試撤銷 (Undo)
void test_recovery_undo() {
    std::cout << "Testing recovery undo..." << std::endl;
    
    std::string data_dir = "./test_recovery_undo";
    
    // 1. 模擬崩潰前，有一個未提交的事務
    {
        StorageEngine storage(data_dir);
        if (!storage.initialize()) abort();
        
        WAL wal(data_dir);
        if (!wal.initialize()) abort();
        
        LockManager lock_mgr;
        TransactionManager txn_mgr(&wal, &lock_mgr, &storage);
        
        Transaction* txn = txn_mgr.begin();
        txn_mgr.put(txn, "key_uncommitted", "should_be_gone");
        // 不提交，直接退出範圍 -> 模擬崩潰
        // 注意：put 操作已經寫入了 WAL 並且 "Put" 到了 storage
        // 下一次啟動時，StorageEngine 是空的（因為內存），Redo 會重做這個 Put，
        // 然後 Undo 應該刪除它。
        
        // 為了驗證 Undo 邏輯，我們必須確保 Redo 確實執行了 Put。
        // 如果 StorageEngine 是內存的，重啟後本來就沒了，Redo 會加進去，Undo 再刪掉。
        // 最終結果應該是沒有。
    }
    
    // 2. 模擬恢復
    {
        StorageEngine storage(data_dir);
        if (!storage.initialize()) abort();
        
        WAL wal(data_dir);
        if (!wal.initialize()) abort();
        
        RecoveryManager recovery(&wal, &storage);
        if (!recovery.recover()) abort();
        
        // 驗證 key_uncommitted 是否不存在
        std::string val;
        if (storage.get("key_uncommitted", val)) {
            std::cerr << "Uncommitted key should have been undone" << std::endl;
            abort();
        }
    }
    
    std::cout << "  ✓ Recovery undo test passed" << std::endl;
}

int main() {
    std::cout << "=== Recovery Manager Test Suite ===" << std::endl << std::endl;
    
    try {
        test_recovery_redo();
        test_recovery_undo();
        
        std::cout << std::endl << "=== All recovery tests passed! ===" << std::endl;
        return 0;
    } catch (...) {
        std::cerr << "Test failed with unknown exception" << std::endl;
        return 1;
    }
}
