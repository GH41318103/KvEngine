#include "kvengine/storage/page_manager.h"
#include <iostream>
#include <sys/stat.h>
#include <cstdio>

namespace kvengine {

PageManager::PageManager(const std::string& db_file) 
    : file_name_(db_file), next_page_id_(0) {}

PageManager::~PageManager() {
    close();
}

bool PageManager::open() {
    // Open for read/update (file must exist)
    db_file_ = fopen(file_name_.c_str(), "rb+");
    if (db_file_ == nullptr) {
        // Create new file
        db_file_ = fopen(file_name_.c_str(), "wb+");
        if (db_file_ == nullptr) {
             std::cerr << "PageManager: Failed to create DB file: " << file_name_ << std::endl;
             return false;
        }
    }

    // Determine file size
    fseek(db_file_, 0, SEEK_END);
    long file_size = ftell(db_file_);
    rewind(db_file_);

    if (file_size % PAGE_SIZE != 0) {
        std::cerr << "PageManager: Warning: DB file size is not multiple of PAGE_SIZE" << std::endl;
    }
    next_page_id_ = static_cast<page_id_t>(file_size / PAGE_SIZE);

    return true;
}

void PageManager::close() {
    if (db_file_) {
        fclose(db_file_);
        db_file_ = nullptr;
    }
}

void PageManager::write_page(page_id_t page_id, const char* data) {
    if (!db_file_) return;
    std::lock_guard<std::mutex> lock(io_mutex_);
    
    long offset = static_cast<long>(page_id) * PAGE_SIZE;
    
    if (fseek(db_file_, offset, SEEK_SET) != 0) {
         std::cerr << "PageManager: fseek failed for write page " << page_id << std::endl;
         return;
    }
    
    size_t written = fwrite(data, 1, PAGE_SIZE, db_file_);
    if (written != PAGE_SIZE) {
        std::cerr << "PageManager: fwrite failed. Written " << written << " bytes" << std::endl;
    }
    fflush(db_file_);
}

void PageManager::read_page(page_id_t page_id, char* data) {
    if (!db_file_) return;
    std::lock_guard<std::mutex> lock(io_mutex_);

    long offset = static_cast<long>(page_id) * PAGE_SIZE;

    if (fseek(db_file_, offset, SEEK_SET) != 0) {
        std::cerr << "PageManager: fseek failed for read page " << page_id << std::endl;
        memset(data, 0, PAGE_SIZE);
        return;
    }
    
    size_t read_count = fread(data, 1, PAGE_SIZE, db_file_);
    if (read_count < PAGE_SIZE) {
        // std::cerr << "PageManager: Read less than PAGE_SIZE for page " << page_id << std::endl;
        // Zero out rest if partial read (or new page)
        memset(data + read_count, 0, PAGE_SIZE - read_count);
    }
}

page_id_t PageManager::allocate_page() {
    return next_page_id_.fetch_add(1);
    // Note: We don't necessarily write to disk immediately, 
    // but the next write_page will potentially extend the file.
}

void PageManager::deallocate_page(page_id_t page_id) {
    (void)page_id;
}

int PageManager::get_num_pages() const {
    return next_page_id_.load();
}

} // namespace kvengine
