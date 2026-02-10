/**
 * @file checkpoint_manager.cpp
 * @brief 檢查點管理器實現
 */

#include "checkpoint_manager.h"
#include <iostream>
#include <sstream>

namespace kvengine {

CheckpointManager::CheckpointManager(WAL* wal, TransactionManager* txn_mgr, StorageEngine* storage)
    : wal_(wal), txn_mgr_(txn_mgr), storage_(storage) {
}

bool CheckpointManager::create_checkpoint() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!wal_ || !txn_mgr_ || !storage_) return false;
    
    std::cout << "Starting checkpoint..." << std::endl;
    
    // 0. 將數據刷盤 (確保檢查點之前的數據已持久化)
    // 這是關鍵步驟，必須在截斷 WAL 之前執行
    if (!storage_->flush()) {
        std::cerr << "Checkpoint failed: storage flush error" << std::endl;
        return false;
    }
    
    // 1. 獲取當前所有活躍事務
    std::vector<Transaction*> active_txns = txn_mgr_->get_active_transactions();
    
    // 2. 構造活躍事務列表字符串（簡單序列化）
    std::stringstream ss;
    for (size_t i = 0; i < active_txns.size(); ++i) {
        ss << active_txns[i]->get_id();
        if (i < active_txns.size() - 1) ss << ",";
    }
    
    // 3. 寫入 CHECKPOINT 日誌記錄
    LogRecord record(LogRecordType::CHECKPOINT, 0, ss.str());
    uint64_t cp_lsn = wal_->append(record);
    wal_->flush();
    
    // 4. 截斷 WAL
    // 找出活躍事務中最老的 LSN
    uint64_t min_lsn = cp_lsn;
    for (auto txn : active_txns) {
        if (txn->get_start_lsn() < min_lsn && txn->get_start_lsn() > 0) {
            min_lsn = txn->get_start_lsn();
        }
    }
    
    std::cout << "Checkpoint created at LSN " << cp_lsn << ", min active LSN " << min_lsn << std::endl;
    
    // 截斷 min_lsn 之前的日誌
    if (min_lsn > 1) {
        wal_->truncate(min_lsn);
    }
    
    return true;
}

} // namespace kvengine
