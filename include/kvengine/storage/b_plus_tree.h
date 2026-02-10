#pragma once

#include "kvengine/storage/buffer_pool_manager.h"
#include "kvengine/storage/b_plus_tree_leaf_page.h"
#include "kvengine/storage/b_plus_tree_internal_page.h"
#include <string>
#include <vector>

namespace kvengine {

// Forward declaration
template <typename KeyType, typename ValueType, typename KeyComparator>
class BPlusTree;

/**
 * BPlusTreeIterator
 * Supports forward traversal of the B+ Tree leaf pages.
 */
template <typename KeyType, typename ValueType, typename KeyComparator>
class BPlusTreeIterator {
public:
    BPlusTreeIterator(BufferPoolManager *bpm, page_id_t page_id, int index = 0) 
        : buffer_pool_manager_(bpm), page_id_(page_id), index_(index), leaf_(nullptr) {
        if (page_id_ != INVALID_PAGE_ID) {
            Page *page = buffer_pool_manager_->fetch_page(page_id_);
            if (page != nullptr) {
                leaf_ = reinterpret_cast<BPlusTreeLeafPage<KeyType, ValueType, KeyComparator> *>(page->get_data());
                // If index is at or past end, move to next page
                if (index_ >= leaf_->get_size()) {
                    page_id_t next_id = leaf_->get_next_page_id();
                    buffer_pool_manager_->unpin_page(page_id_, false);
                    page_id_ = next_id;
                    index_ = 0;
                    leaf_ = nullptr;
                    if (page_id_ != INVALID_PAGE_ID) {
                        page = buffer_pool_manager_->fetch_page(page_id_);
                        if (page != nullptr) {
                            leaf_ = reinterpret_cast<BPlusTreeLeafPage<KeyType, ValueType, KeyComparator> *>(page->get_data());
                        }
                    }
                }
            } else {
                page_id_ = INVALID_PAGE_ID;
            }
        }
    }

    ~BPlusTreeIterator() {
        if (leaf_ != nullptr) {
            buffer_pool_manager_->unpin_page(page_id_, false);
        }
    }

    // Move constructor/assignment to handle resource ownership if needed
    // For simplicity, disable copy to avoid double unpinning
    BPlusTreeIterator(const BPlusTreeIterator&) = delete;
    BPlusTreeIterator& operator=(const BPlusTreeIterator&) = delete;
    
    // Allow move
    BPlusTreeIterator(BPlusTreeIterator&& other) noexcept 
        : buffer_pool_manager_(other.buffer_pool_manager_), 
          page_id_(other.page_id_), 
          index_(other.index_), 
          leaf_(other.leaf_) {
        other.leaf_ = nullptr;
        other.page_id_ = INVALID_PAGE_ID;
    }

    bool is_end() const {
        return page_id_ == INVALID_PAGE_ID;
    }

    KeyType key() const {
        return leaf_->key_at(index_);
    }

    ValueType value() const {
        return leaf_->value_at(index_);
    }

    BPlusTreeIterator& operator++() {
        if (is_end()) return *this;
        
        index_++;
        if (index_ >= leaf_->get_size()) {
            page_id_t next_id = leaf_->get_next_page_id();
            
            // Unpin current
            buffer_pool_manager_->unpin_page(page_id_, false);
            leaf_ = nullptr;
            
            page_id_ = next_id;
            index_ = 0;
            
            if (page_id_ != INVALID_PAGE_ID) {
                Page *page = buffer_pool_manager_->fetch_page(page_id_);
                if (page != nullptr) {
                    leaf_ = reinterpret_cast<BPlusTreeLeafPage<KeyType, ValueType, KeyComparator> *>(page->get_data());
                } else {
                    page_id_ = INVALID_PAGE_ID;
                }
            }
        }
        return *this;
    }

    BufferPoolManager *buffer_pool_manager_;
    page_id_t page_id_;
    int index_;
    BPlusTreeLeafPage<KeyType, ValueType, KeyComparator> *leaf_;
};

// Index Tree
template <typename KeyType, typename ValueType, typename KeyComparator>
class BPlusTree {
public:
    using Iterator = BPlusTreeIterator<KeyType, ValueType, KeyComparator>;

    explicit BPlusTree(std::string index_name, BufferPoolManager *bm, KeyComparator comparator,
                       int leaf_max_size = 0, int internal_max_size = 0);

    // Returns true if this B+ tree has no keys and values.
    bool is_empty() const;

    // Insert a key-value pair into this B+ tree.
    bool insert(const KeyType &key, const ValueType &value);

    // Remove a key and its value from this B+ tree.
    void remove(const KeyType &key);

    // Return the value associated with a given key
    bool get_value(const KeyType &key, std::vector<ValueType> &result);

    // Iterator access
    Iterator begin();
    Iterator begin(const KeyType &key);

private:
    void start_new_tree(const KeyType &key, const ValueType &value);
    
    // Split operations
    bool  insert_into_leaf(const KeyType &key, const ValueType &value);
    void  insert_into_parent(BPlusTreePage *old_node, const KeyType &key, BPlusTreePage *new_node);

    // Helpers
    BPlusTreeLeafPage<KeyType, ValueType, KeyComparator> *split_leaf(BPlusTreeLeafPage<KeyType, ValueType, KeyComparator> *node);
    
    BPlusTreeInternalPage<KeyType, page_id_t, KeyComparator> *split_internal(BPlusTreeInternalPage<KeyType, page_id_t, KeyComparator> *node);

    // Find leaf for a key
    Page *find_leaf_page(const KeyType &key, bool left_most = false);

    std::string index_name_;
    page_id_t root_page_id_;
    BufferPoolManager *buffer_pool_manager_;
    KeyComparator comparator_;
    int leaf_max_size_;
    int internal_max_size_;
    std::mutex latch_; // Tree latch
};

} // namespace kvengine
