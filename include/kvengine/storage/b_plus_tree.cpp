#include "kvengine/storage/b_plus_tree.h"
#include <iostream>

namespace kvengine {

template <typename KeyType, typename ValueType, typename KeyComparator>
BPlusTree<KeyType, ValueType, KeyComparator>::BPlusTree(std::string index_name, BufferPoolManager *bm, KeyComparator comparator, int leaf_max_size, int internal_max_size)
    : index_name_(std::move(index_name)),
      root_page_id_(INVALID_PAGE_ID),
      buffer_pool_manager_(bm),
      comparator_(comparator),
      leaf_max_size_(leaf_max_size),
      internal_max_size_(internal_max_size) {}

template <typename KeyType, typename ValueType, typename KeyComparator>
bool BPlusTree<KeyType, ValueType, KeyComparator>::is_empty() const {
    return root_page_id_ == INVALID_PAGE_ID;
}

template <typename KeyType, typename ValueType, typename KeyComparator>
Page *BPlusTree<KeyType, ValueType, KeyComparator>::find_leaf_page(const KeyType &key, bool left_most) {
    if (is_empty()) return nullptr;
    
    Page *page = buffer_pool_manager_->fetch_page(root_page_id_);
    if (page == nullptr) return nullptr;
    
    BPlusTreePage *node = reinterpret_cast<BPlusTreePage *>(page->get_data());
    
    while (!node->is_leaf_page()) {
        auto *internal = static_cast<BPlusTreeInternalPage<KeyType, page_id_t, KeyComparator> *>(node);
        page_id_t next_page_id;
        
        if (left_most) {
           next_page_id = internal->value_at(0);
        } else {
           next_page_id = internal->lookup(key, comparator_);
        }
        
        buffer_pool_manager_->unpin_page(page->get_page_id(), false);
        page = buffer_pool_manager_->fetch_page(next_page_id);
        if (page == nullptr) return nullptr;
        node = reinterpret_cast<BPlusTreePage *>(page->get_data());
    }
    return page;
}

template <typename KeyType, typename ValueType, typename KeyComparator>
bool BPlusTree<KeyType, ValueType, KeyComparator>::get_value(const KeyType &key, std::vector<ValueType> &result) {
    std::lock_guard<std::mutex> lock(latch_);
    Page *page = find_leaf_page(key);
    if (page == nullptr) return false;

    auto *leaf = reinterpret_cast<BPlusTreeLeafPage<KeyType, ValueType, KeyComparator> *>(page->get_data());
    ValueType val;
    bool found = leaf->lookup(key, val, comparator_);
    if (found) {
        result.push_back(val);
    }
    
    buffer_pool_manager_->unpin_page(page->get_page_id(), false);
    return found;
}

template <typename KeyType, typename ValueType, typename KeyComparator>
typename BPlusTree<KeyType, ValueType, KeyComparator>::Iterator 
BPlusTree<KeyType, ValueType, KeyComparator>::begin() {
    // Get root_page_id atomically without holding lock during BufferPool ops
    page_id_t root_id;
    {
        std::lock_guard<std::mutex> lock(latch_);
        root_id = root_page_id_;
    }
    
    if (root_id == INVALID_PAGE_ID) {
        return Iterator(buffer_pool_manager_, INVALID_PAGE_ID);
    }
    
    // Now traverse without holding tree lock
    Page *page = buffer_pool_manager_->fetch_page(root_id);
    if (page == nullptr) return Iterator(buffer_pool_manager_, INVALID_PAGE_ID);
    
    BPlusTreePage *node = reinterpret_cast<BPlusTreePage *>(page->get_data());
    
    // Traverse to leftmost leaf
    while (!node->is_leaf_page()) {
        auto *internal = static_cast<BPlusTreeInternalPage<KeyType, page_id_t, KeyComparator> *>(node);
        page_id_t next_page_id = internal->value_at(0); // Leftmost child
        
        buffer_pool_manager_->unpin_page(page->get_page_id(), false);
        page = buffer_pool_manager_->fetch_page(next_page_id);
        if (page == nullptr) return Iterator(buffer_pool_manager_, INVALID_PAGE_ID);
        node = reinterpret_cast<BPlusTreePage *>(page->get_data());
    }
    
    page_id_t leaf_id = page->get_page_id();
    buffer_pool_manager_->unpin_page(leaf_id, false);
    
    return Iterator(buffer_pool_manager_, leaf_id, 0);
}

template <typename KeyType, typename ValueType, typename KeyComparator>
typename BPlusTree<KeyType, ValueType, KeyComparator>::Iterator 
BPlusTree<KeyType, ValueType, KeyComparator>::begin(const KeyType &key) {
    // Get root_page_id atomically
    page_id_t root_id;
    {
        std::lock_guard<std::mutex> lock(latch_);
        root_id = root_page_id_;
    }
    
    if (root_id == INVALID_PAGE_ID) {
        return Iterator(buffer_pool_manager_, INVALID_PAGE_ID);
    }
    
    // Traverse without holding tree lock
    Page *page = buffer_pool_manager_->fetch_page(root_id);
    if (page == nullptr) return Iterator(buffer_pool_manager_, INVALID_PAGE_ID);
    
    BPlusTreePage *node = reinterpret_cast<BPlusTreePage *>(page->get_data());
    
    while (!node->is_leaf_page()) {
        auto *internal = static_cast<BPlusTreeInternalPage<KeyType, page_id_t, KeyComparator> *>(node);
        page_id_t next_page_id = internal->lookup(key, comparator_);
        
        buffer_pool_manager_->unpin_page(page->get_page_id(), false);
        page = buffer_pool_manager_->fetch_page(next_page_id);
        if (page == nullptr) return Iterator(buffer_pool_manager_, INVALID_PAGE_ID);
        node = reinterpret_cast<BPlusTreePage *>(page->get_data());
    }
    
    auto *leaf = reinterpret_cast<BPlusTreeLeafPage<KeyType, ValueType, KeyComparator> *>(page->get_data());
    
    // Find index >= key
    int index = 0;
    for (; index < leaf->get_size(); ++index) {
        if (comparator_(leaf->key_at(index), key) >= 0) break;
    }
    
    page_id_t leaf_id = page->get_page_id();
    buffer_pool_manager_->unpin_page(leaf_id, false);
    
    return Iterator(buffer_pool_manager_, leaf_id, index);
}

template <typename KeyType, typename ValueType, typename KeyComparator>
bool BPlusTree<KeyType, ValueType, KeyComparator>::insert(const KeyType &key, const ValueType &value) {
    std::lock_guard<std::mutex> lock(latch_);
    if (is_empty()) {
        start_new_tree(key, value);
        return true;
    }
    
    return insert_into_leaf(key, value);
}

template <typename KeyType, typename ValueType, typename KeyComparator>
void BPlusTree<KeyType, ValueType, KeyComparator>::start_new_tree(const KeyType &key, const ValueType &value) {
    page_id_t page_id;
    Page *page = buffer_pool_manager_->new_page(&page_id);
    root_page_id_ = page_id;
    auto *root = reinterpret_cast<BPlusTreeLeafPage<KeyType, ValueType, KeyComparator> *>(page->get_data());
    root->init(root_page_id_, INVALID_PAGE_ID, leaf_max_size_);
    root->insert(key, value, comparator_);
    buffer_pool_manager_->unpin_page(page_id, true);
}

template <typename KeyType, typename ValueType, typename KeyComparator>
bool BPlusTree<KeyType, ValueType, KeyComparator>::insert_into_leaf(const KeyType &key, const ValueType &value) {
    Page *page = buffer_pool_manager_->fetch_page(root_page_id_);
    BPlusTreePage *node = reinterpret_cast<BPlusTreePage *>(page->get_data());
    
    while (!node->is_leaf_page()) {
        auto *internal = static_cast<BPlusTreeInternalPage<KeyType, page_id_t, KeyComparator> *>(node);
        page_id_t next_page_id = internal->lookup(key, comparator_);
        
        buffer_pool_manager_->unpin_page(page->get_page_id(), false);
        page = buffer_pool_manager_->fetch_page(next_page_id);
        node = reinterpret_cast<BPlusTreePage *>(page->get_data());
    }
    
    auto *leaf = static_cast<BPlusTreeLeafPage<KeyType, ValueType, KeyComparator> *>(node);
    
    if (leaf->get_size() < leaf->get_max_size()) {
        leaf->insert(key, value, comparator_);
        buffer_pool_manager_->unpin_page(page->get_page_id(), true);
        return true;
    }
    
    auto *new_leaf = split_leaf(leaf);
    
    if (comparator_(key, new_leaf->key_at(0)) < 0) {
        leaf->insert(key, value, comparator_);
    } else {
        new_leaf->insert(key, value, comparator_);
    }
    
    insert_into_parent(leaf, new_leaf->key_at(0), new_leaf);
    
    buffer_pool_manager_->unpin_page(leaf->get_page_id(), true);
    buffer_pool_manager_->unpin_page(new_leaf->get_page_id(), true);
    return true;
}

template <typename KeyType, typename ValueType, typename KeyComparator>
BPlusTreeLeafPage<KeyType, ValueType, KeyComparator> *
BPlusTree<KeyType, ValueType, KeyComparator>::split_leaf(BPlusTreeLeafPage<KeyType, ValueType, KeyComparator> *old_leaf) {
    page_id_t new_page_id;
    Page *new_page = buffer_pool_manager_->new_page(&new_page_id);
    auto *new_leaf = reinterpret_cast<BPlusTreeLeafPage<KeyType, ValueType, KeyComparator> *>(new_page->get_data());
    
    new_leaf->init(new_page_id, old_leaf->get_parent_page_id(), leaf_max_size_);
    old_leaf->move_half_to(new_leaf);
    
    new_leaf->set_next_page_id(old_leaf->get_next_page_id());
    old_leaf->set_next_page_id(new_page_id);
    
    return new_leaf;
}

template <typename KeyType, typename ValueType, typename KeyComparator>
BPlusTreeInternalPage<KeyType, page_id_t, KeyComparator> *
BPlusTree<KeyType, ValueType, KeyComparator>::split_internal(BPlusTreeInternalPage<KeyType, page_id_t, KeyComparator> *old_internal) {
    page_id_t new_page_id;
    Page *new_page = buffer_pool_manager_->new_page(&new_page_id);
    auto *new_internal = reinterpret_cast<BPlusTreeInternalPage<KeyType, page_id_t, KeyComparator> *>(new_page->get_data());
    
    new_internal->init(new_page_id, old_internal->get_parent_page_id(), internal_max_size_);
    
    old_internal->move_half_to(new_internal, buffer_pool_manager_);
    
    return new_internal;
}

template <typename KeyType, typename ValueType, typename KeyComparator>
void BPlusTree<KeyType, ValueType, KeyComparator>::insert_into_parent(BPlusTreePage *old_node, const KeyType &key, BPlusTreePage *new_node) {
    if (old_node->is_root_page()) {
        page_id_t new_root_id;
        Page *new_root_page = buffer_pool_manager_->new_page(&new_root_id);
        
        auto *new_root = reinterpret_cast<BPlusTreeInternalPage<KeyType, page_id_t, KeyComparator> *>(new_root_page->get_data());
        new_root->init(new_root_id, INVALID_PAGE_ID, internal_max_size_);
        new_root->populate_new_root(old_node->get_page_id(), key, new_node->get_page_id());
        
        old_node->set_parent_page_id(new_root_id);
        new_node->set_parent_page_id(new_root_id);
        
        root_page_id_ = new_root_id;
        buffer_pool_manager_->unpin_page(new_root_id, true);
        return;
    }
    
    page_id_t parent_id = old_node->get_parent_page_id();
    Page *parent_page = buffer_pool_manager_->fetch_page(parent_id);
    auto *parent = reinterpret_cast<BPlusTreeInternalPage<KeyType, page_id_t, KeyComparator> *>(parent_page->get_data());
    
    if (parent->get_size() < parent->get_max_size()) {
        parent->insert_node_after(old_node->get_page_id(), key, new_node->get_page_id());
        buffer_pool_manager_->unpin_page(parent_id, true);
        return;
    }
    
    // Parent is full, need to split
    // Split the parent first
    auto *new_parent = split_internal(parent);
    
    // The first key in new_parent is the middle key to push up
    KeyType middle_key = new_parent->key_at(0);
    
    // Now insert the new entry into the correct parent node
    // If key < middle_key, insert into old parent; otherwise into new parent
    if (comparator_(key, middle_key) < 0) {
        parent->insert_node_after(old_node->get_page_id(), key, new_node->get_page_id());
        new_node->set_parent_page_id(parent_id);
    } else {
        new_parent->insert_node_after(old_node->get_page_id(), key, new_node->get_page_id());
        new_node->set_parent_page_id(new_parent->get_page_id());
    }
    
    // Recursively insert middle_key into grandparent
    insert_into_parent(parent, middle_key, new_parent);
    
    buffer_pool_manager_->unpin_page(parent_id, true);
    buffer_pool_manager_->unpin_page(new_parent->get_page_id(), true);
}

template <typename KeyType, typename ValueType, typename KeyComparator>
void BPlusTree<KeyType, ValueType, KeyComparator>::remove(const KeyType &key) {
    (void)key;
}

} // namespace kvengine
