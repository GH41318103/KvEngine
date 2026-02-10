/**
 * @file recovery_manager.cpp
 * @brief 恢復管理器實現
 */

#include "recovery_manager.h"
#include <iostream>
#include <algorithm>

namespace kvengine {

RecoveryManager::RecoveryManager(WAL* wal, StorageEngine* storage)
    : wal_(wal), storage_(storage) {
}

bool RecoveryManager::recover() {
    std::cout << "Starting recovery..." << std::endl;
    
    if (!wal_ || !storage_) return false;
    
    // 1. 讀取所有日誌記錄
    auto records = wal_->read_from(0);
    if (records.empty()) {
        std::cout << "No logs found, recovery skipped." << std::endl;
        return true;
    }
    
    std::cout << "Read " << records.size() << " log records." << std::endl;
    
    // 2. 分析階段：確定 Winners (已提交) 和 Losers (未提交)
    std::set<uint64_t> active_txns;
    std::set<uint64_t> committed_txns;
    std::set<uint64_t> aborted_txns;
    
    for (const auto& record : records) {
        if (record.type == LogRecordType::BEGIN) {
            active_txns.insert(record.txn_id);
        } else if (record.type == LogRecordType::COMMIT) {
            active_txns.erase(record.txn_id);
            committed_txns.insert(record.txn_id);
        } else if (record.type == LogRecordType::ROLLBACK) {
            active_txns.erase(record.txn_id);
            aborted_txns.insert(record.txn_id);
        }
    }
    
    std::cout << "Analysis complete: " << active_txns.size() << " active, " 
              << committed_txns.size() << " committed, " 
              << aborted_txns.size() << " aborted." << std::endl;
    
    // 3. 重做階段 (Redo)：重放所有操作以恢復狀態
    // 注意：這裡採取簡單的 "Repeat History" 策略
    redo(records);
    
    // 4. 撤銷階段 (Undo)：回滾未提交的事務
    if (!active_txns.empty()) {
        undo(records, active_txns);
    }
    
    std::cout << "Recovery completed." << std::endl;
    return true;
}

void RecoveryManager::redo(const std::vector<LogRecord>& records) {
    std::cout << "Redoing operations..." << std::endl;
    int redo_count = 0;
    
    for (const auto& record : records) {
        if (record.type == LogRecordType::PUT) {
            storage_->put(record.key, record.value);
            redo_count++;
        } else if (record.type == LogRecordType::DELETE) {
            storage_->remove(record.key);
            redo_count++;
        }
    }
    
    std::cout << "Redone " << redo_count << " operations." << std::endl;
}

void RecoveryManager::undo(const std::vector<LogRecord>& records, const std::set<uint64_t>& loser_txns) {
    std::cout << "Undoing " << loser_txns.size() << " loser transactions..." << std::endl;
    int undo_count = 0;
    
    // 反向掃描日誌進行撤銷
    for (auto it = records.rbegin(); it != records.rend(); ++it) {
        const auto& record = *it;
        
        // 如果是 loser 事務的操作
        if (loser_txns.count(record.txn_id)) {
            if (record.type == LogRecordType::PUT) {
                // 撤銷 PUT：執行刪除 (有限制，無法恢復舊值)
                // 假設 TransactionManager::rollback 的語義是刪除
                storage_->remove(record.key);
                undo_count++;
            } else if (record.type == LogRecordType::DELETE) {
                // 撤銷 DELETE：無法恢復，因为日誌沒有記錄被刪除的值
                std::cerr << "Warning: Cannot undo DELETE for txn " << record.txn_id 
                          << ", key " << record.key << " (missing old value)" << std::endl;
            }
        }
    }
    
    std::cout << "Undone " << undo_count << " operations." << std::endl;
}

} // namespace kvengine
