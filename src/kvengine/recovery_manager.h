/**
 * @file recovery_manager.h
 * @brief 恢復管理器定義
 * @details 負責數據庫崩潰恢復
 */

#ifndef KVENGINE_RECOVERY_MANAGER_H
#define KVENGINE_RECOVERY_MANAGER_H

#include <vector>
#include <set>
#include <map>
#include "wal.h"
#include "storage_engine.h"

namespace kvengine {

/**
 * @class RecoveryManager
 * @brief 恢復管理器
 */
class RecoveryManager {
public:
    /**
     * @brief 構造函數
     * @param wal WAL 指針
     * @param storage 存儲引擎指針
     */
    RecoveryManager(WAL* wal, StorageEngine* storage);
    
    /**
     * @brief 執行恢復
     * @details 分析日誌，重做已提交事務，回滾未提交事務
     * @return 成功返回 true
     */
    bool recover();

private:
    /**
     * @brief 重做階段
     * @param records 日誌記錄列表
     */
    void redo(const std::vector<LogRecord>& records);
    
    /**
     * @brief 撤銷階段
     * @param records 日誌記錄列表
     * @param loser_txns 未提交的事務 ID 集合
     */
    void undo(const std::vector<LogRecord>& records, const std::set<uint64_t>& loser_txns);

    WAL* wal_;
    StorageEngine* storage_;
};

} // namespace kvengine

#endif // KVENGINE_RECOVERY_MANAGER_H
