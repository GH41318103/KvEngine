/**
 * @file storage_engine.h
 * @brief 存儲引擎實現
 * @details 負責數據的實際存儲、序列化和持久化
 */

#ifndef KVENGINE_STORAGE_ENGINE_H
#define KVENGINE_STORAGE_ENGINE_H

#include "../include/kvengine/types.h"
#include <string>
#include <map>
#include <fstream>
#include <mutex>

namespace kvengine {

/**
 * @class StorageEngine
 * @brief 數據存儲引擎
 * @details 提供數據的內存存儲和磁盤持久化功能
 *          - 使用 std::map 作為內存存儲
 *          - 支持二進制序列化
 *          - 線程安全
 */
class StorageEngine {
public:
    /**
     * @brief 構造函數
     * @param data_dir 數據目錄路徑
     */
    explicit StorageEngine(const std::string& data_dir);
    
    /**
     * @brief 析構函數
     * @details 自動刷新數據到磁盤
     */
    ~StorageEngine();
    
    /**
     * @brief 初始化存儲引擎
     * @return 成功返回 true，失敗返回 false
     * @details 創建數據目錄，加載已有數據
     */
    bool initialize();
    
    /**
     * @brief 插入或更新鍵值對
     * @param key 鍵
     * @param value 值
     * @return 成功返回 true
     */
    bool put(const std::string& key, const std::string& value);
    
    /**
     * @brief 獲取鍵對應的值
     * @param key 鍵
     * @param value 輸出參數，存儲獲取的值
     * @return 找到返回 true，未找到返回 false
     */
    bool get(const std::string& key, std::string& value) const;
    
    /**
     * @brief 刪除鍵值對
     * @param key 要刪除的鍵
     * @return 成功返回 true，鍵不存在返回 false
     */
    bool remove(const std::string& key);
    
    /**
     * @brief 檢查鍵是否存在
     * @param key 要檢查的鍵
     * @return 存在返回 true，不存在返回 false
     */
    bool exists(const std::string& key) const;
    
    /**
     * @brief 獲取所有數據（用於迭代）
     * @return 數據 map 的常量引用
     */
    const std::map<std::string, std::string>& get_all_data() const;
    
    /**
     * @brief 刷新數據到磁盤
     * @return 成功返回 true，失敗返回 false
     */
    bool flush();
    
    /**
     * @brief 從磁盤加載數據
     * @return 成功返回 true，失敗返回 false
     */
    bool load();
    
    /**
     * @brief 獲取存儲的鍵值對數量
     * @return 鍵值對數量
     */
    size_t size() const;
    
    /**
     * @brief 獲取內存使用量
     * @return 內存使用量（字節）
     */
    size_t memory_usage() const;
    
private:
    std::string data_dir_;                           // 數據目錄
    std::string data_file_;                          // 數據文件路徑
    std::map<std::string, std::string> data_;        // 內存中的數據
    mutable std::mutex mutex_;                       // 線程安全鎖
    
    // 序列化相關
    /**
     * @brief 將數據序列化到文件
     * @param filename 文件名
     * @return 成功返回 true
     */
    bool serialize_to_file(const std::string& filename);
    
    /**
     * @brief 從文件反序列化數據
     * @param filename 文件名
     * @return 成功返回 true
     */
    bool deserialize_from_file(const std::string& filename);
    
    /**
     * @brief 獲取數據文件完整路徑
     * @return 數據文件路徑
     */
    std::string get_data_file_path() const;
};

} // namespace kvengine

#endif // KVENGINE_STORAGE_ENGINE_H
