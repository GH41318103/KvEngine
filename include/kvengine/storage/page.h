#pragma once

#include <cstdint>
#include <cstring>
#include <iostream>

namespace kvengine {

// Standard Page Size 4KB
static constexpr int PAGE_SIZE = 4096;
static constexpr uint32_t INVALID_PAGE_ID = UINT32_MAX;

using page_id_t = uint32_t;
using lsn_t = uint64_t;     // Log Sequence Number

// Page Type
enum class PageType : uint8_t {
    UNKNOWN = 0,
    INTERNAL_PAGE = 1,
    LEAF_PAGE = 2,
    HEADER_PAGE = 3,  // Metadata
    FREE_PAGE = 4     // Free list
};

/**
 * Page Class.
 * Represents a standard 4KB page in memory.
 */
class Page {
    friend class PageManager;
    friend class BufferPoolManager;

public:
    Page() { reset_memory(); }
    ~Page() = default;

    inline char* get_data() { return data_; }
    inline const char* get_data() const { return data_; }

    inline page_id_t get_page_id() const { return page_id_; }
    
    // Metadata accessors (can be stored in page data header)
    inline lsn_t get_lsn() const { 
        return *reinterpret_cast<const lsn_t*>(data_ + OFFSET_LSN); 
    }
    
    inline void set_lsn(lsn_t lsn) {
        memcpy(data_ + OFFSET_LSN, &lsn, sizeof(lsn_t));
    }

    inline void set_page_id(page_id_t page_id) { page_id_ = page_id; }
    
    inline bool is_dirty() const { return is_dirty_; }
    inline void set_dirty(bool dirty) { is_dirty_ = dirty; }
    
    inline int get_pin_count() const { return pin_count_; }
    inline void pin() { pin_count_++; }
    inline void unpin() { if(pin_count_ > 0) pin_count_--; }

    void reset_memory() { memset(data_, 0, PAGE_SIZE); }

private:
    // Page Header offsets within data_
    static constexpr size_t OFFSET_LSN = 0;   // 0-8: LSN
    static constexpr size_t OFFSET_TYPE = 8;  // 8-9: Type (padded to 4?)
    // Actually, let specific page types (BPlusTreePage) handle internal structure.
    // Base Page just holds raw data.

    char data_[PAGE_SIZE];
    page_id_t page_id_ = INVALID_PAGE_ID;
    int pin_count_ = 0;
    bool is_dirty_ = false;
};

} // namespace kvengine
