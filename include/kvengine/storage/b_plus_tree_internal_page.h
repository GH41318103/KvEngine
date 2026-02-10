#pragma once

#include "kvengine/storage/b_plus_tree_page.h"
#include <cstring>
#include <algorithm>

namespace kvengine {

#define B_PLUS_TREE_INTERNAL_PAGE_TYPE BPlusTreeInternalPage<KeyType, ValueType, KeyComparator>

template <typename KeyType, typename ValueType, typename KeyComparator>
class BPlusTreeInternalPage : public BPlusTreePage {
public:
    void init(page_id_t page_id, page_id_t parent_id = INVALID_PAGE_ID, int max_size = 0) {
        set_page_type(IndexPageType::INTERNAL_PAGE);
        set_size(0);
        set_max_size(max_size);
        set_parent_page_id(parent_id);
        set_page_id(page_id);
    }
    
    KeyType key_at(int index) const {
        return array_[index].first;
    }
    
    void set_key_at(int index, const KeyType &key) {
        array_[index].first = key;
    }
    
    ValueType value_at(int index) const {
        return array_[index].second;
    }
    
    void set_value_at(int index, const ValueType &value) {
        array_[index].second = value;
    }
    
    ValueType lookup(const KeyType &key, const KeyComparator &comparator) const {
        int l = 1, r = get_size() - 1;
        int target = 0; // Default to index 0
        while (l <= r) {
            int mid = l + (r - l) / 2;
            if (comparator(array_[mid].first, key) <= 0) { // key >= array[mid].first
                target = mid;
                l = mid + 1;
            } else {
                r = mid - 1;
            }
        }
        return array_[target].second;
    }

    void populate_new_root(const ValueType &old_value, const KeyType &new_key, const ValueType &new_value) {
        array_[0].second = old_value;
        array_[1].first = new_key;
        array_[1].second = new_value;
        set_size(2);
    }
    
    int insert_node_after(const ValueType &old_value, const KeyType &new_key, const ValueType &new_value) {
        // Find index of old_value
        int idx = 0;
        for (; idx < get_size(); ++idx) {
            if (value_at(idx) == old_value) break;
        }
        
        // Shift right
        for (int i = get_size(); i > idx + 1; --i) {
            array_[i] = array_[i - 1];
        }
        
        array_[idx + 1] = {new_key, new_value};
        increase_size(1);
        return get_size();
    }
    
    // Move half to recipient
    void move_half_to(BPlusTreeInternalPage *recipient, BufferPoolManager *buffer_pool_manager) {
        int start_idx = get_min_size(); // Usually split at mid
        int move_count = get_size() - start_idx;
        recipient->copy_n_from(array_ + start_idx, move_count, buffer_pool_manager);
        set_size(start_idx);
    }
    
    void copy_n_from(std::pair<KeyType, ValueType> *items, int size, BufferPoolManager *buffer_pool_manager) {
        // Update parent pointers of children being moved!
        for (int i = 0; i < size; ++i) {
             array_[i] = items[i];
             // The child pages now belong to this new internal page
             Page *child_raw = buffer_pool_manager->fetch_page(items[i].second);
             if (child_raw != nullptr) {
                 auto *child = reinterpret_cast<BPlusTreePage *>(child_raw->get_data());
                 child->set_parent_page_id(get_page_id());
                 buffer_pool_manager->unpin_page(child->get_page_id(), true);
             }
        }
        set_size(size);
    }

private:
    std::pair<KeyType, ValueType> array_[1]; 
};

} // namespace kvengine
