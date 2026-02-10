#pragma once

#include <vector>
#include <list>
#include <mutex>
#include <unordered_map>
#include "kvengine/storage/page_manager.h"
#include "kvengine/storage/page.h"

namespace kvengine {

using frame_id_t = int32_t;

/**
 * BufferPoolManager manages the memory frames and the mapping from internal page_id to frame_id.
 * It uses the LRU replacement policy.
 */
class BufferPoolManager {
public:
    BufferPoolManager(size_t pool_size, PageManager* page_manager);
    ~BufferPoolManager();

    // Fetch a page from the buffer pool. 
    // If it's not in the pool, read it from disk.
    // If the pool is full, evict a victim page.
    Page* fetch_page(page_id_t page_id);

    // Unpin a page, indicating the thread is done with it.
    // If is_dirty is true, the page should be marked as dirty.
    bool unpin_page(page_id_t page_id, bool is_dirty);

    // Flush a specific page to disk.
    bool flush_page(page_id_t page_id);

    // Create a new page in the buffer pool.
    // Returns both the Page pointer and the new page_id.
    Page* new_page(page_id_t* page_id);

    // Delete a page from the buffer pool and disk.
    bool delete_page(page_id_t page_id);

    // Flush all dirty pages to disk.
    void flush_all_pages();

private:
    // Helper to find a victim frame to replace.
    // Returns true if a victim is found, false otherwise (all pinned).
    bool find_victim(frame_id_t* frame_id);

    size_t pool_size_;
    PageManager* page_manager_;
    std::vector<Page*> pages_; // The frames
    std::list<frame_id_t> free_list_; // Frames that are empty
    std::unordered_map<page_id_t, frame_id_t> page_table_; // Map page_id -> frame_id

    // Simple LRU: Use a list (or clock algorithm). 
    // For simplicity, we can use a std::list for LRU order or keep timestamps.
    // Let's use a replacer component concept, but inline for now.
    // List of frame_ids. Front = LRU, Back = MRU.
    std::list<frame_id_t> lru_list_; 
    std::unordered_map<frame_id_t, std::list<frame_id_t>::iterator> lru_map_;

    std::mutex latch_;
};

} // namespace kvengine
