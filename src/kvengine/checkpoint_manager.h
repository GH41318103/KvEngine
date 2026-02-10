/**
 * @file checkpoint_manager.h
 * @brief 檢查點管理器
 * @details 負責創建檢查點，優化恢復速度並截斷 WAL
 */

#ifndef KVENGINE_CHECKPOINT_MANAGER_H
#define KVENGINE_CHECKPOINT_MANAGER_H

#include <string>
#include <mutex>
#include <vector>
#include "wal.h"
#include "transaction_manager.h"
#include "storage_engine.h"

namespace kvengine {

/**
 * @class CheckpointManager
 * @brief 檢查點管理器
 */
class CheckpointManager {
public:
    /**
     * @brief 構造函數
     * @param wal WAL 指針
     * @param txn_mgr 事務管理器指針
     * @param storage 存儲引擎指針
     */
    CheckpointManager(WAL* wal, TransactionManager* txn_mgr, StorageEngine* storage);
    
    /**
     * @brief 執行檢查點
     * @details 將內存數據刷盤，記錄活躍事務，並截斷 WAL
     * @return 成功返回 true
     */
    bool create_checkpoint();
    
private:
    WAL* wal_;
    TransactionManager* txn_mgr_;
    StorageEngine* storage_;
    std::mutex mutex_;
};

} // namespace kvengine

#endif // KVENGINE_CHECKPOINT_MANAGER_H
