#include "../src/kvengine/transaction_manager.h"
#include "../src/kvengine/storage_engine.h"
#include <iostream>
#include <cassert>

using namespace kvengine;

// 測試事務提交
void test_transaction_commit() {
    std::cout << "Testing transaction commit..." << std::endl;
    
    std::string data_dir = "./test_txn_commit";
    StorageEngine storage(data_dir);
    if (!storage.initialize()) {
        std::cerr << "Failed to initialize storage" << std::endl;
        abort();
    }
    
    WAL wal(data_dir);
    if (!wal.initialize()) {
        std::cerr << "Failed to initialize WAL" << std::endl;
        abort();
    }
    
    LockManager lock_mgr;
    TransactionManager txn_mgr(&wal, &lock_mgr, &storage);
    
    // 開始事務
    Transaction* txn = txn_mgr.begin();
    if (!txn) {
        std::cerr << "Failed to begin transaction" << std::endl;
        abort();
    }
    
    // 執行寫入
    if (!txn_mgr.put(txn, "key1", "value1")) {
        std::cerr << "Failed to put key1" << std::endl;
        abort();
    }
    if (!txn_mgr.put(txn, "key2", "value2")) {
        std::cerr << "Failed to put key2" << std::endl;
        abort();
    }
    
    // 驗證存儲中已更新
    std::string val;
    storage.get("key1", val);
    if (val != "value1") {
        std::cerr << "Value mismatch for key1" << std::endl;
        abort();
    }
    
    // 提交事務
    std::cout << "Committing transaction..." << std::endl;
    if (!txn_mgr.commit(txn)) {
        std::cerr << "Failed to commit transaction" << std::endl;
        abort();
    }
    
    // 驗證 WAL 中有記錄
    std::cout << "Reading WAL..." << std::endl;
    auto records = wal.read_from(0);
    std::cout << "WAL records: " << records.size() << std::endl;
    if (records.size() < 4) {
        std::cerr << "WAL records size mismatch: " << records.size() << std::endl;
        abort();
    }
    
    delete txn;
    std::cout << "  ✓ Transaction commit test passed" << std::endl;
}

// 測試事務回滾
void test_transaction_rollback() {
    std::cout << "Testing transaction rollback..." << std::endl;
    
    std::string data_dir = "./test_txn_rollback";
    StorageEngine storage(data_dir);
    if (!storage.initialize()) abort();
    
    WAL wal(data_dir);
    if (!wal.initialize()) abort();
    
    LockManager lock_mgr;
    TransactionManager txn_mgr(&wal, &lock_mgr, &storage);
    
    // 寫入初始數據
    storage.put("key1", "original");
    
    // 開始事務
    Transaction* txn = txn_mgr.begin();
    
    // 更新數據
    txn_mgr.put(txn, "key1", "modified");
    txn_mgr.put(txn, "key2", "new_val");
    
    // 回滾事務
    if (!txn_mgr.rollback(txn)) abort();
    
    // 驗證數據已回滾
    std::string val;
    if (storage.get("key2", val)) abort();
    
    delete txn;
    std::cout << "  ✓ Transaction rollback test passed" << std::endl;
}

// 測試事務併發和鎖
void test_transaction_concurrency() {
    std::cout << "Testing transaction concurrency..." << std::endl;
    
    std::string data_dir = "./test_txn_concurrency";
    StorageEngine storage(data_dir);
    if (!storage.initialize()) abort();
    
    WAL wal(data_dir);
    if (!wal.initialize()) abort();
    
    LockManager lock_mgr;
    TransactionManager txn_mgr(&wal, &lock_mgr, &storage);
    
    // 事務 1 開始並持有 key1 的排他鎖
    Transaction* txn1 = txn_mgr.begin();
    txn_mgr.put(txn1, "key1", "val1");
    
    // 事務 2 嘗試寫入 key1 應該會失敗（非阻塞測試）
    Transaction* txn2 = txn_mgr.begin();
    if (lock_mgr.try_lock(txn2->get_id(), "key1", LockMode::EXCLUSIVE)) abort();
    
    // 事務 1 提交後，事務 2 應該可以獲取鎖
    if (!txn_mgr.commit(txn1)) abort();
    if (!lock_mgr.try_lock(txn2->get_id(), "key1", LockMode::EXCLUSIVE)) abort();
    
    if (!txn_mgr.commit(txn2)) abort();
    
    delete txn1;
    delete txn2;
    std::cout << "  ✓ Transaction concurrency test passed" << std::endl;
}

// 主測試函數
int main() {
    std::cout << "=== Transaction Manager Test Suite ===" << std::endl << std::endl;
    
    try {
        std::cout << "Starting commit test..." << std::endl;
        test_transaction_commit();
        std::cout << "Commit test finished." << std::endl;
        
        std::cout << "Starting rollback test..." << std::endl;
        test_transaction_rollback();
        std::cout << "Rollback test finished." << std::endl;
        
        std::cout << "Starting concurrency test..." << std::endl;
        test_transaction_concurrency();
        std::cout << "Concurrency test finished." << std::endl;
        
        std::cout << std::endl << "=== All transaction tests passed! ===" << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Test failed with unknown exception" << std::endl;
        return 1;
    }
}
