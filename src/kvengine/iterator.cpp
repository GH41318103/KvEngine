#include "../include/kvengine/iterator.h"
#include <algorithm>

namespace kvengine {

MapIterator::MapIterator(const std::map<std::string, std::string>& data,
                         const std::string& prefix)
    : prefix_(prefix) {
    // Create a copy of the data for iteration
    data_copy_ = std::make_shared<std::map<std::string, std::string>>(data);
    
    if (prefix.empty()) {
        iter_ = data_copy_->begin();
        end_ = data_copy_->end();
    } else {
        // Find first key with prefix
        iter_ = data_copy_->lower_bound(prefix);
        end_ = data_copy_->end();
        
        // Skip to first matching key
        while (iter_ != end_ && !matches_prefix()) {
            ++iter_;
        }
    }
}

bool MapIterator::valid() const {
    return iter_ != end_ && matches_prefix();
}

void MapIterator::next() {
    if (iter_ != end_) {
        ++iter_;
    }
}

std::string MapIterator::key() const {
    if (valid()) {
        return iter_->first;
    }
    return "";
}

std::string MapIterator::value() const {
    if (valid()) {
        return iter_->second;
    }
    return "";
}

void MapIterator::seek(const std::string& target) {
    iter_ = std::find_if(iter_, end_, [&target](const auto& pair) {
        return pair.first >= target;
    });
}

void MapIterator::seek_to_first() {
    // This would need access to the original data
    // For now, this is a placeholder
}

bool MapIterator::matches_prefix() const {
    if (iter_ == end_) {
        return false;
    }
    
    if (prefix_.empty()) {
        return true;
    }
    
    const std::string& key = iter_->first;
    return key.size() >= prefix_.size() &&
           key.compare(0, prefix_.size(), prefix_) == 0;
}

} // namespace kvengine
