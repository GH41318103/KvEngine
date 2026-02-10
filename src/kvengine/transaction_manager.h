/**
 * @file transaction_manager.h
 * @brief 事務管理器實現
 * @details 管理所有事務的生命週期
 */

#ifndef KVENGINE_TRANSACTION_MANAGER_H
#define KVENGINE_TRANSACTION_MANAGER_H

#include <kvengine/transaction.h>
#include "wal.h"
#include "lock_manager.h"
#include <map>
#include <mutex>
#include <atomic>
#include <memory>

namespace kvengine {

// 前向聲明
class StorageEngine;

/**
 * @class TransactionManager
 * @brief 事務管理器
 * @details 負責事務的創建、提交和回滾
 */
class TransactionManager {
public:
    /**
     * @brief 構造函數
     * @param wal WAL 指針
     * @param lock_mgr 鎖管理器指針
     * @param storage 存儲引擎指針
     */
    TransactionManager(WAL* wal, LockManager* lock_mgr, StorageEngine* storage);
    
    /**
     * @brief 析構函數
     */
    ~TransactionManager();
    
    /**
     * @brief 開始新事務
     * @return 事務指針
     */
    Transaction* begin();
    
    /**
     * @brief 提交事務
     * @param txn 事務指針
     * @return 成功返回 true
     */
    bool commit(Transaction* txn);
    
    /**
     * @brief 回滾事務
     * @param txn 事務指針
     * @return 成功返回 true
     */
    bool rollback(Transaction* txn);
    
    /**
     * @brief 獲取當前活躍事務
     * @return 活躍事務列表
     */
    std::vector<Transaction*> get_active_transactions();
    
    /**
     * @brief 在事務中插入鍵值對
     * @param txn 事務指針
     * @param key 鍵
     * @param value 值
     * @return 成功返回 true
     */
    bool put(Transaction* txn, const std::string& key, const std::string& value);
    
    /**
     * @brief 在事務中刪除鍵
     * @param txn 事務指針
     * @param key 鍵
     * @return 成功返回 true
     */
    bool remove(Transaction* txn, const std::string& key);
    
private:
    WAL* wal_;                                      // WAL 指針
    LockManager* lock_mgr_;                         // 鎖管理器指針
    StorageEngine* storage_;                        // 存儲引擎指針
    std::atomic<uint64_t> next_txn_id_;             // 下一個事務 ID
    std::map<uint64_t, Transaction*> active_txns_;  // 活躍事務表
    std::mutex mutex_;                              // 線程安全鎖
};

} // namespace kvengine

#endif // KVENGINE_TRANSACTION_MANAGER_H
