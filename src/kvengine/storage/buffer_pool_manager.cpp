#include "kvengine/storage/buffer_pool_manager.h"
#include <iostream>

namespace kvengine {

BufferPoolManager::BufferPoolManager(size_t pool_size, PageManager* page_manager)
    : pool_size_(pool_size), page_manager_(page_manager) {
    
    // Allocate pages array
    pages_.resize(pool_size_);
    for (size_t i = 0; i < pool_size_; ++i) {
        pages_[i] = new Page();
        free_list_.push_back(static_cast<frame_id_t>(i));
    }
}

BufferPoolManager::~BufferPoolManager() {
    flush_all_pages();
    for (size_t i = 0; i < pool_size_; ++i) {
        delete pages_[i];
    }
}

bool BufferPoolManager::find_victim(frame_id_t* frame_id) {
    if (lru_list_.empty()) {
        return false;
    }
    // Simple LRU: evict from front
    *frame_id = lru_list_.front();
    lru_list_.pop_front();
    lru_map_.erase(*frame_id);
    return true;
}

Page* BufferPoolManager::fetch_page(page_id_t page_id) {
    std::lock_guard<std::mutex> lock(latch_);

    // 1. Check if page is already in pool
    if (page_table_.find(page_id) != page_table_.end()) {
        frame_id_t frame_id = page_table_[page_id];
        Page* page = pages_[frame_id];
        
        page->pin();
        
        // Remove from LRU list while pinned
        if (lru_map_.find(frame_id) != lru_map_.end()) {
            lru_list_.erase(lru_map_[frame_id]);
            lru_map_.erase(frame_id);
        }
        
        return page;
    }

    // 2. Find a frame for replacement
    frame_id_t frame_id = -1;
    if (!free_list_.empty()) {
        frame_id = free_list_.front();
        free_list_.pop_front();
    } else {
        if (!find_victim(&frame_id)) {
            return nullptr; // All pages pinned
        }
        
        // Write back victim page if dirty
        Page* victim_page = pages_[frame_id];
        if (victim_page->is_dirty()) {
            page_manager_->write_page(victim_page->get_page_id(), victim_page->get_data());
        }
        page_table_.erase(victim_page->get_page_id());
    }

    // 3. Read page from disk
    Page* page = pages_[frame_id];
    page->reset_memory();
    page->set_page_id(page_id);
    page->set_dirty(false);
    page->pin_count_ = 1; // Pinned immediately
    
    page_manager_->read_page(page_id, page->get_data());
    
    page_table_[page_id] = frame_id;

    return page;
}

bool BufferPoolManager::unpin_page(page_id_t page_id, bool is_dirty) {
    std::lock_guard<std::mutex> lock(latch_);
    
    if (page_table_.find(page_id) == page_table_.end()) {
        return false;
    }
    
    frame_id_t frame_id = page_table_[page_id];
    Page* page = pages_[frame_id];
    
    if (is_dirty) {
        page->set_dirty(true);
    }
    
    if (page->get_pin_count() > 0) {
        page->unpin();
    }
    
    if (page->get_pin_count() == 0) {
        // Add to LRU list (MRU position: back)
        if (lru_map_.find(frame_id) == lru_map_.end()) {
            lru_list_.push_back(frame_id);
            lru_map_[frame_id] = --lru_list_.end();
        }
    }
    
    return true;
}

bool BufferPoolManager::flush_page(page_id_t page_id) {
    std::lock_guard<std::mutex> lock(latch_);
    if (page_table_.find(page_id) == page_table_.end()) return false;
    
    frame_id_t frame_id = page_table_[page_id];
    Page* page = pages_[frame_id];
    
    page_manager_->write_page(page_id, page->get_data());
    page->set_dirty(false);
    return true;
}

Page* BufferPoolManager::new_page(page_id_t* page_id) {
    std::lock_guard<std::mutex> lock(latch_);
    
    frame_id_t frame_id = -1;
    if (!free_list_.empty()) {
        frame_id = free_list_.front();
        free_list_.pop_front();
    } else {
        if (!find_victim(&frame_id)) return nullptr;
        
        Page* victim = pages_[frame_id];
        if (victim->is_dirty()) {
            page_manager_->write_page(victim->get_page_id(), victim->get_data());
        }
        page_table_.erase(victim->get_page_id());
    }
    
    *page_id = page_manager_->allocate_page();
    Page* page = pages_[frame_id];
    page->reset_memory();
    page->set_page_id(*page_id);
    page->set_dirty(false);
    page->pin_count_ = 1;
    
    page_table_[*page_id] = frame_id;
    return page;
}

bool BufferPoolManager::delete_page(page_id_t page_id) {
    std::lock_guard<std::mutex> lock(latch_);
    if (page_table_.find(page_id) == page_table_.end()) return true;
    
    frame_id_t frame_id = page_table_[page_id];
    Page* page = pages_[frame_id];
    
    if (page->get_pin_count() > 0) return false; // Cannot delete pinned page
    
    // Remove from LRU
    if (lru_map_.find(frame_id) != lru_map_.end()) {
        lru_list_.erase(lru_map_[frame_id]);
        lru_map_.erase(frame_id);
    }
    
    page_table_.erase(page_id);
    page->reset_memory();
    page->set_page_id(INVALID_PAGE_ID);
    page->set_dirty(false);
    
    free_list_.push_back(frame_id);
    page_manager_->deallocate_page(page_id);
    return true;
}

void BufferPoolManager::flush_all_pages() {
    std::lock_guard<std::mutex> lock(latch_);
    for (auto& pair : page_table_) {
        Page* page = pages_[pair.second];
        if (page->is_dirty()) {
            page_manager_->write_page(page->get_page_id(), page->get_data());
            page->set_dirty(false);
        }
    }
}

} // namespace kvengine
