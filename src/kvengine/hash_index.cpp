#include "hash_index.h"
#include <algorithm>

namespace kvengine {

HashIndex::HashIndex() {
}

HashIndex::~HashIndex() {
}

void HashIndex::insert(const std::string& key, size_t offset) {
    std::lock_guard<std::mutex> lock(mutex_);
    index_[key] = offset;
}

bool HashIndex::lookup(const std::string& key, size_t& offset) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = index_.find(key);
    if (it != index_.end()) {
        offset = it->second;
        return true;
    }
    return false;
}

bool HashIndex::remove(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    return index_.erase(key) > 0;
}

bool HashIndex::exists(const std::string& key) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return index_.find(key) != index_.end();
}

std::vector<std::string> HashIndex::get_keys_with_prefix(const std::string& prefix) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> result;
    
    for (const auto& pair : index_) {
        const std::string& key = pair.first;
        if (key.size() >= prefix.size() &&
            key.compare(0, prefix.size(), prefix) == 0) {
            result.push_back(key);
        }
    }
    
    // Sort results for consistent ordering
    std::sort(result.begin(), result.end());
    
    return result;
}

std::vector<std::string> HashIndex::get_all_keys() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> result;
    
    for (const auto& pair : index_) {
        result.push_back(pair.first);
    }
    
    // Sort results for consistent ordering
    std::sort(result.begin(), result.end());
    
    return result;
}

void HashIndex::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    index_.clear();
}

size_t HashIndex::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return index_.size();
}

} // namespace kvengine
