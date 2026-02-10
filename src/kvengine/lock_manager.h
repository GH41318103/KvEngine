/**
 * @file lock_manager.h
 * @brief 鎖管理器實現
 * @details 提供行級鎖支持，實現並發控制
 */

#ifndef KVENGINE_LOCK_MANAGER_H
#define KVENGINE_LOCK_MANAGER_H

#include <string>
#include <map>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <cstdint>

namespace kvengine {

// 前向聲明
class Transaction;

/**
 * @enum LockMode
 * @brief 鎖模式
 */
enum class LockMode {
    SHARED,      // 共享鎖（讀鎖）
    EXCLUSIVE    // 排他鎖（寫鎖）
};

/**
 * @struct LockRequest
 * @brief 鎖請求結構
 */
struct LockRequest {
    uint64_t txn_id;        // 事務 ID
    LockMode mode;          // 鎖模式
    bool granted;           // 是否已授予
    
    LockRequest(uint64_t tid, LockMode m)
        : txn_id(tid), mode(m), granted(false) {}
};

/**
 * @class LockManager
 * @brief 鎖管理器
 * @details 管理所有鍵的鎖，支持共享鎖和排他鎖
 */
class LockManager {
public:
    /**
     * @brief 構造函數
     */
    LockManager();
    
    /**
     * @brief 析構函數
     */
    ~LockManager();
    
    /**
     * @brief 獲取共享鎖（讀鎖）
     * @param txn_id 事務 ID
     * @param key 鍵
     * @return 成功返回 true
     */
    bool lock_shared(uint64_t txn_id, const std::string& key);
    
    /**
     * @brief 獲取排他鎖（寫鎖）
     * @param txn_id 事務 ID
     * @param key 鍵
     * @return 成功返回 true
     */
    bool lock_exclusive(uint64_t txn_id, const std::string& key);
    
    /**
     * @brief 釋放鎖
     * @param txn_id 事務 ID
     * @param key 鍵
     * @return 成功返回 true
     */
    bool unlock(uint64_t txn_id, const std::string& key);
    
    /**
     * @brief 釋放事務的所有鎖
     * @param txn_id 事務 ID
     * @return 成功返回 true
     */
    bool unlock_all(uint64_t txn_id);
    
    /**
     * @brief 嘗試獲取鎖（非阻塞）
     * @param txn_id 事務 ID
     * @param key 鍵
     * @param mode 鎖模式
     * @return 成功返回 true，失敗返回 false
     */
    bool try_lock(uint64_t txn_id, const std::string& key, LockMode mode);
    
private:
    // 鎖表：鍵 -> 鎖請求列表
    std::map<std::string, std::vector<LockRequest>> lock_table_;
    
    // 事務持有的鎖：事務 ID -> 鍵列表
    std::map<uint64_t, std::vector<std::string>> txn_locks_;
    
    // 全局鎖
    std::mutex mutex_;
    
    // 條件變量（用於等待鎖）
    std::condition_variable cv_;
    
    /**
     * @brief 檢查是否可以授予鎖
     * @param key 鍵
     * @param mode 鎖模式
     * @param txn_id 事務 ID
     * @return 可以授予返回 true
     */
    bool can_grant_lock(const std::string& key, LockMode mode, uint64_t txn_id);
    
    /**
     * @brief 授予鎖
     * @param key 鍵
     * @param txn_id 事務 ID
     */
    void grant_lock(const std::string& key, uint64_t txn_id);
};

} // namespace kvengine

#endif // KVENGINE_LOCK_MANAGER_H
