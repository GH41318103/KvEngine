/**
 * @file lock_manager.cpp
 * @brief 鎖管理器實現文件
 */

#include "lock_manager.h"
#include <iostream>
#include <algorithm>

namespace kvengine {

LockManager::LockManager() {
}

LockManager::~LockManager() {
}

bool LockManager::lock_shared(uint64_t txn_id, const std::string& key) {
    std::unique_lock<std::mutex> lock(mutex_);
    
    // 檢查是否可以立即授予鎖
    if (can_grant_lock(key, LockMode::SHARED, txn_id)) {
        // 添加鎖請求
        LockRequest request(txn_id, LockMode::SHARED);
        request.granted = true;
        lock_table_[key].push_back(request);
        
        // 記錄事務持有的鎖
        txn_locks_[txn_id].push_back(key);
        
        return true;
    }
    
    // 需要等待，添加到等待隊列
    LockRequest request(txn_id, LockMode::SHARED);
    lock_table_[key].push_back(request);
    
    // 等待鎖被授予
    cv_.wait(lock, [this, &key, txn_id]() {
        auto& requests = lock_table_[key];
        for (auto& req : requests) {
            if (req.txn_id == txn_id && req.granted) {
                return true;
            }
        }
        return false;
    });
    
    // 記錄事務持有的鎖
    txn_locks_[txn_id].push_back(key);
    
    return true;
}

bool LockManager::lock_exclusive(uint64_t txn_id, const std::string& key) {
    std::unique_lock<std::mutex> lock(mutex_);
    
    // 檢查是否可以立即授予鎖
    if (can_grant_lock(key, LockMode::EXCLUSIVE, txn_id)) {
        // 添加鎖請求
        LockRequest request(txn_id, LockMode::EXCLUSIVE);
        request.granted = true;
        lock_table_[key].push_back(request);
        
        // 記錄事務持有的鎖
        txn_locks_[txn_id].push_back(key);
        
        return true;
    }
    
    // 需要等待，添加到等待隊列
    LockRequest request(txn_id, LockMode::EXCLUSIVE);
    lock_table_[key].push_back(request);
    
    // 等待鎖被授予
    cv_.wait(lock, [this, &key, txn_id]() {
        auto& requests = lock_table_[key];
        for (auto& req : requests) {
            if (req.txn_id == txn_id && req.granted) {
                return true;
            }
        }
        return false;
    });
    
    // 記錄事務持有的鎖
    txn_locks_[txn_id].push_back(key);
    
    return true;
}

bool LockManager::unlock(uint64_t txn_id, const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 從鎖表中移除
    auto it = lock_table_.find(key);
    if (it == lock_table_.end()) {
        return false;
    }
    
    auto& requests = it->second;
    requests.erase(
        std::remove_if(requests.begin(), requests.end(),
            [txn_id](const LockRequest& req) {
                return req.txn_id == txn_id;
            }),
        requests.end()
    );
    
    // 如果沒有更多請求，移除鍵
    if (requests.empty()) {
        lock_table_.erase(it);
    } else {
        // 嘗試授予等待中的鎖
        for (auto& req : requests) {
            if (!req.granted && can_grant_lock(key, req.mode, req.txn_id)) {
                grant_lock(key, req.txn_id);
            }
        }
    }
    
    // 從事務鎖列表中移除
    auto txn_it = txn_locks_.find(txn_id);
    if (txn_it != txn_locks_.end()) {
        auto& keys = txn_it->second;
        keys.erase(std::remove(keys.begin(), keys.end(), key), keys.end());
        
        if (keys.empty()) {
            txn_locks_.erase(txn_it);
        }
    }
    
    // 通知等待的線程
    cv_.notify_all();
    
    return true;
}

bool LockManager::unlock_all(uint64_t txn_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = txn_locks_.find(txn_id);
    if (it == txn_locks_.end()) {
        return true;  // 沒有持有任何鎖
    }
    
    // 複製鍵列表（因為 unlock 會修改它）
    auto keys = it->second;
    
    // 釋放所有鎖
    for (const auto& key : keys) {
        auto lock_it = lock_table_.find(key);
        if (lock_it != lock_table_.end()) {
            auto& requests = lock_it->second;
            requests.erase(
                std::remove_if(requests.begin(), requests.end(),
                    [txn_id](const LockRequest& req) {
                        return req.txn_id == txn_id;
                    }),
                requests.end()
            );
            
            // 如果沒有更多請求，移除鍵
            if (requests.empty()) {
                lock_table_.erase(lock_it);
            } else {
                // 嘗試授予等待中的鎖
                for (auto& req : requests) {
                    if (!req.granted && can_grant_lock(key, req.mode, req.txn_id)) {
                        grant_lock(key, req.txn_id);
                    }
                }
            }
        }
    }
    
    // 移除事務記錄
    txn_locks_.erase(it);
    
    // 通知等待的線程
    cv_.notify_all();
    
    return true;
}

bool LockManager::try_lock(uint64_t txn_id, const std::string& key, LockMode mode) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 檢查是否可以立即授予鎖
    if (!can_grant_lock(key, mode, txn_id)) {
        return false;
    }
    
    // 添加鎖請求
    LockRequest request(txn_id, mode);
    request.granted = true;
    lock_table_[key].push_back(request);
    
    // 記錄事務持有的鎖
    txn_locks_[txn_id].push_back(key);
    
    return true;
}

bool LockManager::can_grant_lock(const std::string& key, LockMode mode, uint64_t txn_id) {
    auto it = lock_table_.find(key);
    if (it == lock_table_.end()) {
        // 沒有任何鎖，可以授予
        return true;
    }
    
    const auto& requests = it->second;
    
    // 檢查已授予的鎖
    for (const auto& req : requests) {
        if (!req.granted) {
            continue;  // 跳過未授予的請求
        }
        
        if (req.txn_id == txn_id) {
            // 同一個事務已經持有鎖
            // 如果已經有排他鎖，可以授予任何鎖
            if (req.mode == LockMode::EXCLUSIVE) {
                return true;
            }
            // 如果已經有共享鎖，只能再授予共享鎖
            if (mode == LockMode::SHARED) {
                return true;
            }
            // 嘗試升級鎖（共享 -> 排他）
            // 檢查是否只有這一個共享鎖
            int shared_count = 0;
            for (const auto& r : requests) {
                if (r.granted && r.mode == LockMode::SHARED) {
                    shared_count++;
                }
            }
            return shared_count == 1;
        }
        
        // 其他事務持有鎖
        if (mode == LockMode::EXCLUSIVE || req.mode == LockMode::EXCLUSIVE) {
            // 排他鎖與任何鎖衝突
            return false;
        }
        // 共享鎖與共享鎖兼容
    }
    
    return true;
}

void LockManager::grant_lock(const std::string& key, uint64_t txn_id) {
    auto& requests = lock_table_[key];
    for (auto& req : requests) {
        if (req.txn_id == txn_id && !req.granted) {
            req.granted = true;
            break;
        }
    }
}

} // namespace kvengine
