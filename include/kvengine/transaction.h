/**
 * @file transaction.h
 * @brief 事務接口定義
 * @details 提供事務管理功能
 */

#ifndef KVENGINE_TRANSACTION_H
#define KVENGINE_TRANSACTION_H

#include <string>
#include <vector>
#include <cstdint>

namespace kvengine {

/**
 * @enum TransactionState
 * @brief 事務狀態
 */
enum class TransactionState {
    RUNNING,     // 運行中
    COMMITTED,   // 已提交
    ABORTED      // 已回滾
};

/**
 * @class Transaction
 * @brief 事務對象
 * @details 表示一個數據庫事務
 */
class Transaction {
public:
    /**
     * @brief 構造函數
     * @param txn_id 事務 ID
     */
    explicit Transaction(uint64_t txn_id);
    
    /**
     * @brief 析構函數
     */
    ~Transaction();
    
    /**
     * @brief 獲取事務 ID
     * @return 事務 ID
     */
    uint64_t get_id() const { return txn_id_; }
    
    /**
     * @brief 獲取事務狀態
     * @return 事務狀態
     */
    TransactionState get_state() const { return state_; }
    
    /**
     * @brief 設置事務狀態
     * @param state 新狀態
     */
    void set_state(TransactionState state) { state_ = state; }
    
    /**
     * @brief 記錄寫入的鍵（用於回滾）
     * @param key 鍵
     */
    void add_write_key(const std::string& key);
    
    /**
     * @brief 獲取所有寫入的鍵
     * @return 鍵列表
     */
    const std::vector<std::string>& get_write_keys() const { return write_keys_; }
    
    /**
     * @brief 設置起始 LSN
     * @param lsn LSN
     */
    void set_start_lsn(uint64_t lsn) { start_lsn_ = lsn; }
    
    /**
     * @brief 獲取起始 LSN
     * @return LSN
     */
    uint64_t get_start_lsn() const { return start_lsn_; }
    
private:
    uint64_t txn_id_;                    // 事務 ID
    TransactionState state_;             // 事務狀態
    std::vector<std::string> write_keys_;// 寫入的鍵列表
    uint64_t start_lsn_;                 // 起始 LSN
};

} // namespace kvengine

#endif // KVENGINE_TRANSACTION_H
