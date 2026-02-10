#ifndef KVENGINE_MEMORY_MANAGER_H
#define KVENGINE_MEMORY_MANAGER_H

#include <cstddef>
#include <atomic>
#include <mutex>

namespace kvengine {

// 內存管理器，用於跟踪內存使用情況
class MemoryManager {
public:
    MemoryManager();
    explicit MemoryManager(size_t max_memory);
    ~MemoryManager();
    
    // 分配內存
    bool allocate(size_t size);
    
    // 釋放內存
    void deallocate(size_t size);
    
    // 獲取當前內存使用量
    size_t get_memory_usage() const;
    
    // 獲取最大內存限制
    size_t get_max_memory() const;
    
    // 設置最大內存限制
    void set_max_memory(size_t max_memory);
    
    // 檢查分配是否會超過限制
    bool can_allocate(size_t size) const;
    
    // 重置內存使用量
    void reset();
    
private:
    std::atomic<size_t> current_memory_;  // 當前內存使用量（原子操作）
    size_t max_memory_;                   // 最大內存限制
    mutable std::mutex mutex_;            // 線程安全鎖
};

} // namespace kvengine

#endif // KVENGINE_MEMORY_MANAGER_H
