/**
 * @file kv_engine.h
 * @brief KvEngine 主引擎接口
 * @details 提供鍵值存儲的完整 API，包括 CRUD 操作、批量操作、迭代器等
 */

#ifndef KVENGINE_KV_ENGINE_H
#define KVENGINE_KV_ENGINE_H

#include "types.h"
#include "iterator.h"
#include <string>
#include <map>
#include <memory>

namespace kvengine {

// 前向聲明
class StorageEngine;
class HashIndex;
class MemoryManager;

/**
 * @class KvEngine
 * @brief 主鍵值存儲引擎類
 * @details 提供完整的鍵值存儲功能，包括：
 *          - CRUD 操作（創建、讀取、更新、刪除）
 *          - 批量操作
 *          - 迭代器支持
 *          - 數據持久化
 *          - 統計信息
 */
class KvEngine {
public:
    /**
     * @brief 構造函數
     * @param data_dir 數據存儲目錄路徑
     */
    explicit KvEngine(const std::string& data_dir);
    
    /**
     * @brief 析構函數
     * @details 自動關閉引擎並清理資源
     */
    ~KvEngine();
    
    // 禁用拷貝構造和賦值
    KvEngine(const KvEngine&) = delete;
    KvEngine& operator=(const KvEngine&) = delete;
    
    /**
     * @brief 打開引擎
     * @return 成功返回 true，失敗返回 false
     * @details 初始化存儲引擎，加載已有數據，構建索引
     */
    bool open();
    
    /**
     * @brief 關閉引擎
     * @details 刷新數據到磁盤，釋放資源
     */
    void close();
    
    /**
     * @brief 檢查引擎是否已打開
     * @return 已打開返回 true，否則返回 false
     */
    bool is_open() const;
    
    // ===== CRUD 操作 =====
    
    /**
     * @brief 插入或更新鍵值對
     * @param key 鍵
     * @param value 值
     * @return 成功返回 true，失敗返回 false
     */
    bool put(const std::string& key, const std::string& value);
    
    /**
     * @brief 獲取鍵對應的值
     * @param key 鍵
     * @return 值的字符串，如果鍵不存在則返回空字符串
     */
    std::string get(const std::string& key);
    
    /**
     * @brief 獲取鍵對應的值（帶狀態返回）
     * @param key 鍵
     * @param value 輸出參數，存儲獲取的值
     * @return 操作狀態
     */
    Status get(const std::string& key, std::string& value);
    
    /**
     * @brief 刪除鍵值對
     * @param key 要刪除的鍵
     * @return 成功返回 true，失敗返回 false
     */
    bool remove(const std::string& key);
    
    /**
     * @brief 檢查鍵是否存在
     * @param key 要檢查的鍵
     * @return 存在返回 true，不存在返回 false
     */
    bool exists(const std::string& key);
    
    // ===== 批量操作 =====
    
    /**
     * @brief 批量插入鍵值對
     * @param batch 鍵值對的 map
     * @return 成功返回 true，失敗返回 false
     */
    bool batch_put(const std::map<std::string, std::string>& batch);
    
    // ===== 迭代器操作 =====
    
    /**
     * @brief 創建迭代器進行掃描
     * @param prefix 可選的前綴過濾，默認為空（掃描所有鍵）
     * @return 迭代器的智能指針
     */
    std::unique_ptr<Iterator> scan(const std::string& prefix = "");
    
    // ===== 統計信息 =====
    
    /**
     * @brief 獲取引擎統計信息
     * @return 統計信息結構
     */
    Statistics get_statistics() const;
    
    // ===== 持久化 =====
    
    /**
     * @brief 將數據刷新到磁盤
     * @return 成功返回 true，失敗返回 false
     */
    bool flush();
    
    /**
     * @brief 驗證數據完整性
     * @return 數據完整返回 true，否則返回 false
     */
    bool verify_integrity();
    
private:
    class Impl;                      // 前向聲明內部實現類
    std::unique_ptr<Impl> impl_;     // Pimpl 模式的實現指針
};

} // namespace kvengine

#endif // KVENGINE_KV_ENGINE_H
