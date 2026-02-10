#pragma once

#include "kvengine/storage/page.h"
#include <string>

namespace kvengine {

// We need a fixed key type and value type for simplicity first. 
// For a general KV store, we usually support variable length keys, 
// but implementing B+Tree with variable length keys inside pages is complex (Slotted Page).
// Let's start with Fixed Size Keys (e.g., int64_t) and Values (Fixed Size) 
// OR implement a basic structure that can be extended.

// Let's assume Key = int64_t for now to get the logic right, 
// creating a B+Tree<int64_t, int64_t>. 
// Later we can wrap strings into this or use a separate index.

// Wait, the user wants "StorageEngine" replacement. The current engine uses std::string keys.
// Implementing variable-length B+Tree is hard. 
// A common simplification is:
// 1. Key is string, max length 64 bytes?
// 2. Value is string, stored in separate Heap File if large?

// Let's stick to a generic BPlusTreePage header first.

enum class IndexPageType { INVALID_INDEX_PAGE = 0, LEAF_PAGE = 1, INTERNAL_PAGE = 2 };

/**
 * BPlusTreePage is the base class for both Internal and Leaf pages.
 * It contains the common header information.
 * 
 * Header Format (size in bytes):
 * ----------------------------------------------------------------------------
 * | PageType (4) | LSN (8) | CurrentSize (4) | MaxSize (4) | ParentPageId (4) |
 * ----------------------------------------------------------------------------
 * Total Header Size = 24 bytes
 */
class BPlusTreePage {
public:
    inline bool is_leaf_page() const { return page_type_ == IndexPageType::LEAF_PAGE; }
    inline bool is_root_page() const { return parent_page_id_ == INVALID_PAGE_ID; }
    
    inline void set_page_type(IndexPageType page_type) { page_type_ = page_type; }
    inline IndexPageType get_page_type() const { return page_type_; }

    inline void set_size(int size) { size_ = size; }
    inline int get_size() const { return size_; }
    inline void increase_size(int amount) { size_ += amount; }

    inline void set_max_size(int max_size) { max_size_ = max_size; }
    inline int get_max_size() const { return max_size_; }
    inline int get_min_size() const { return max_size_ / 2; }

    inline void set_parent_page_id(page_id_t parent_page_id) { parent_page_id_ = parent_page_id; }
    inline page_id_t get_parent_page_id() const { return parent_page_id_; }
    
    inline void set_page_id(page_id_t page_id) { page_id_ = page_id; }
    inline page_id_t get_page_id() const { return page_id_; }
    
    inline void set_lsn(lsn_t lsn) { lsn_ = lsn; }
    inline lsn_t get_lsn() const { return lsn_; }

protected:
    IndexPageType page_type_;
    lsn_t lsn_;
    int size_;
    int max_size_;
    page_id_t parent_page_id_;
    page_id_t page_id_;
    // Flexible array member would follow here...
};

} // namespace kvengine
