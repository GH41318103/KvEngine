#pragma once

#include <string>
#include <fstream>
#include <mutex>
#include <atomic>
#include "kvengine/storage/page.h"

namespace kvengine {

/**
 * PageManager is responsible for reading and writing pages to disk.
 * It manages the underlying file and page allocation.
 */
class PageManager {
public:
    explicit PageManager(const std::string& db_file);
    ~PageManager();

    // Open/Create the database file
    bool open();
    void close();

    // Write a page to disk
    void write_page(page_id_t page_id, const char* data);

    // Read a page from disk
    void read_page(page_id_t page_id, char* data);

    // Allocate a new page ID (safely increments counter)
    page_id_t allocate_page();

    // Deallocate page (for future free list management)
    void deallocate_page(page_id_t page_id);

    // Get current file size in pages
    int get_num_pages() const;

private:
    std::string file_name_;
    FILE* db_file_ = nullptr;
    std::mutex io_mutex_;
    std::atomic<page_id_t> next_page_id_;
};

} // namespace kvengine
