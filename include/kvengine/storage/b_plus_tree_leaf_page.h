#pragma once

#include "kvengine/storage/b_plus_tree_page.h"

namespace kvengine {

#define B_PLUS_TREE_LEAF_PAGE_TYPE BPlusTreeLeafPage<KeyType, ValueType, KeyComparator>

template <typename KeyType, typename ValueType, typename KeyComparator>
class BPlusTreeLeafPage : public BPlusTreePage {
public:
    void init(page_id_t page_id, page_id_t parent_id = INVALID_PAGE_ID, int max_size = 0) {
        set_page_type(IndexPageType::LEAF_PAGE);
        set_size(0);
        set_max_size(max_size);
        set_parent_page_id(parent_id);
        set_page_id(page_id);
        next_page_id_ = INVALID_PAGE_ID;
    }

    page_id_t get_next_page_id() const { return next_page_id_; }
    void set_next_page_id(page_id_t next_page_id) { next_page_id_ = next_page_id; }

    KeyType key_at(int index) const {
        return array_[index].first;
    }
    
    ValueType value_at(int index) const {
        return array_[index].second;
    }
    
    bool insert(const KeyType &key, const ValueType &value, const KeyComparator &comparator) {
        // Find insertion point
        int l = 0, r = get_size();
        while (l < r) {
            int mid = l + (r - l) / 2;
            if (comparator(array_[mid].first, key) < 0) {
                l = mid + 1;
            } else {
                r = mid;
            }
        }
        int target = l;
        
        if (target < get_size() && comparator(array_[target].first, key) == 0) {
            return false; // Duplicate
        }
        
        for (int i = get_size(); i > target; --i) {
            array_[i] = array_[i - 1];
        }
        array_[target] = {key, value};
        increase_size(1);
        return true;
    }
    
    bool lookup(const KeyType &key, ValueType &value, const KeyComparator &comparator) const {
        int l = 0, r = get_size();
        while (l < r) {
            int mid = l + (r - l) / 2;
            if (comparator(array_[mid].first, key) < 0) {
                l = mid + 1;
            } else {
                r = mid;
            }
        }
        if (l < get_size() && comparator(array_[l].first, key) == 0) {
            value = array_[l].second;
            return true;
        }
        return false;
    }

    // Split Helpers
    void move_half_to(BPlusTreeLeafPage *recipient) {
        int start_idx = get_min_size(); 
        int move_count = get_size() - start_idx;
        recipient->copy_n_from(array_ + start_idx, move_count);
        
        set_size(start_idx);
    }
    
    void copy_n_from(std::pair<KeyType, ValueType> *items, int size) {
        for (int i = 0; i < size; ++i) {
            array_[i] = items[i];
        }
        set_size(size);
    }

private:
    page_id_t next_page_id_;
    std::pair<KeyType, ValueType> array_[1];
};

} // namespace kvengine
