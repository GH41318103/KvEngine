#ifndef KVENGINE_HASH_INDEX_H
#define KVENGINE_HASH_INDEX_H

#include <string>
#include <unordered_map>
#include <vector>
#include <mutex>

namespace kvengine {

// 哈希索引，提供 O(1) 的查找性能
class HashIndex {
public:
    HashIndex();
    ~HashIndex();
    
    // 插入或更新鍵
    void insert(const std::string& key, size_t offset);
    
    // 查找鍵，返回是否找到以及偏移量
    bool lookup(const std::string& key, size_t& offset) const;
    
    // 刪除鍵
    bool remove(const std::string& key);
    
    // 檢查鍵是否存在
    bool exists(const std::string& key) const;
    
    // 獲取所有具有指定前綴的鍵
    std::vector<std::string> get_keys_with_prefix(const std::string& prefix) const;
    
    // 獲取所有鍵
    std::vector<std::string> get_all_keys() const;
    
    // 清空索引
    void clear();
    
    // 獲取索引大小
    size_t size() const;
    
private:
    std::unordered_map<std::string, size_t> index_;  // 內部哈希表
    mutable std::mutex mutex_;                       // 線程安全鎖
};

} // namespace kvengine

#endif // KVENGINE_HASH_INDEX_H
