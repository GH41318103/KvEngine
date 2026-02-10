#include "memory_manager.h"
#include <algorithm>

namespace kvengine {

MemoryManager::MemoryManager()
    : current_memory_(0), max_memory_(1024 * 1024 * 1024) { // Default 1GB
}

MemoryManager::MemoryManager(size_t max_memory)
    : current_memory_(0), max_memory_(max_memory) {
}

MemoryManager::~MemoryManager() {
}

bool MemoryManager::allocate(size_t size) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (max_memory_ > 0 && current_memory_ + size > max_memory_) {
        return false;
    }
    
    current_memory_ += size;
    return true;
}

void MemoryManager::deallocate(size_t size) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (current_memory_ >= size) {
        current_memory_ -= size;
    } else {
        current_memory_ = 0;
    }
}

size_t MemoryManager::get_memory_usage() const {
    return current_memory_.load();
}

size_t MemoryManager::get_max_memory() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return max_memory_;
}

void MemoryManager::set_max_memory(size_t max_memory) {
    std::lock_guard<std::mutex> lock(mutex_);
    max_memory_ = max_memory;
}

bool MemoryManager::can_allocate(size_t size) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return max_memory_ == 0 || current_memory_ + size <= max_memory_;
}

void MemoryManager::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    current_memory_ = 0;
}

} // namespace kvengine
