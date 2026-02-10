/**
 * @file transaction_manager.cpp
 * @brief 事務管理器實現文件
 */

#include "transaction_manager.h"
#include "storage_engine.h"
#include <iostream>

namespace kvengine {

TransactionManager::TransactionManager(WAL* wal, LockManager* lock_mgr, StorageEngine* storage)
    : wal_(wal),
      lock_mgr_(lock_mgr),
      storage_(storage),
      next_txn_id_(1) {
}

TransactionManager::~TransactionManager() {
    // 清理所有活躍事務
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& pair : active_txns_) {
        delete pair.second;
    }
    active_txns_.clear();
}

Transaction* TransactionManager::begin() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 分配事務 ID
    uint64_t txn_id = next_txn_id_++;
    
    // 創建事務對象
    Transaction* txn = new Transaction(txn_id);
    
    // 寫入 BEGIN 日誌
    if (wal_) {
        LogRecord record(LogRecordType::BEGIN, txn_id, "");
        uint64_t lsn = wal_->append(record);
        txn->set_start_lsn(lsn);
    }
    
    // 添加到活躍事務表
    active_txns_[txn_id] = txn;
    
    return txn;
}

bool TransactionManager::commit(Transaction* txn) {
    if (!txn || txn->get_state() != TransactionState::RUNNING) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 寫入 COMMIT 日誌
    if (wal_) {
        LogRecord record(LogRecordType::COMMIT, txn->get_id(), "");
        wal_->append(record);
        wal_->flush();  // 確保日誌持久化
    }
    
    // 釋放所有鎖
    if (lock_mgr_) {
        lock_mgr_->unlock_all(txn->get_id());
    }
    
    // 更新事務狀態
    txn->set_state(TransactionState::COMMITTED);
    
    // 從活躍事務表中移除
    active_txns_.erase(txn->get_id());
    
    return true;
}

bool TransactionManager::rollback(Transaction* txn) {
    if (!txn || txn->get_state() != TransactionState::RUNNING) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 回滾所有寫入操作
    const auto& write_keys = txn->get_write_keys();
    for (auto it = write_keys.rbegin(); it != write_keys.rend(); ++it) {
        // 這裡簡化處理：直接刪除鍵
        // 完整實現應該恢復到事務開始前的值
        if (storage_) {
            storage_->remove(*it);
        }
    }
    
    // 寫入 ROLLBACK 日誌
    if (wal_) {
        LogRecord record(LogRecordType::ROLLBACK, txn->get_id(), "");
        wal_->append(record);
        wal_->flush();
    }
    
    // 釋放所有鎖
    if (lock_mgr_) {
        lock_mgr_->unlock_all(txn->get_id());
    }
    
    // 更新事務狀態
    txn->set_state(TransactionState::ABORTED);
    
    // 從活躍事務表中移除
    active_txns_.erase(txn->get_id());
    
    return true;
}

std::vector<Transaction*> TransactionManager::get_active_transactions() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<Transaction*> result;
    for (const auto& pair : active_txns_) {
        result.push_back(pair.second);
    }
    
    return result;
}

bool TransactionManager::put(Transaction* txn, const std::string& key, const std::string& value) {
    if (!txn || txn->get_state() != TransactionState::RUNNING) {
        return false;
    }
    
    // 獲取排他鎖
    if (lock_mgr_ && !lock_mgr_->lock_exclusive(txn->get_id(), key)) {
        return false;
    }
    
    // 寫入 WAL
    if (wal_) {
        LogRecord record(LogRecordType::PUT, txn->get_id(), key, value);
        wal_->append(record);
    }
    
    // 更新存儲
    if (storage_ && !storage_->put(key, value)) {
        return false;
    }
    
    // 記錄寫入的鍵
    txn->add_write_key(key);
    
    return true;
}

bool TransactionManager::remove(Transaction* txn, const std::string& key) {
    if (!txn || txn->get_state() != TransactionState::RUNNING) {
        return false;
    }
    
    // 獲取排他鎖
    if (lock_mgr_ && !lock_mgr_->lock_exclusive(txn->get_id(), key)) {
        return false;
    }
    
    // 寫入 WAL
    if (wal_) {
        LogRecord record(LogRecordType::DELETE, txn->get_id(), key);
        wal_->append(record);
    }
    
    // 更新存儲
    if (storage_ && !storage_->remove(key)) {
        return false;
    }
    
    // 記錄寫入的鍵
    txn->add_write_key(key);
    
    return true;
}

} // namespace kvengine
