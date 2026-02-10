#include <kvengine/storage/buffer_pool_manager.h>
#include <kvengine/storage/page.h>
#include <iostream>
#include <cassert>
#include <cstring>
#include <cstdio>
#include <vector>

using namespace kvengine;

void test_buffer_pool() {
    std::string db_file = "test_bpm.db";
    std::remove(db_file.c_str());

    PageManager* pm = new PageManager(db_file);
    assert(pm->open());

    // Pool size 10
    BufferPoolManager* bpm = new BufferPoolManager(10, pm);
    
    // 1. Create new page
    page_id_t p1_id;
    Page* p1 = bpm->new_page(&p1_id);
    assert(p1 != nullptr);
    assert(p1_id == 0);
    
    strcpy(p1->get_data(), "Hello BufferPool");
    bpm->unpin_page(p1_id, true); // Mark dirty

    // 2. Fetch page (should be in memory)
    Page* p1_fetched = bpm->fetch_page(p1_id);
    assert(p1_fetched == p1);
    assert(strcmp(p1_fetched->get_data(), "Hello BufferPool") == 0);
    bpm->unpin_page(p1_id, false);

    // 3. Fill the pool (create 9 more pages, total 10)
    std::vector<page_id_t> pages;
    pages.push_back(p1_id);
    for (int i = 0; i < 9; ++i) {
        page_id_t pid;
        Page* p = bpm->new_page(&pid);
        assert(p != nullptr);
        pages.push_back(pid);
        bpm->unpin_page(pid, false);
    }
    
    // Now pool is full (10 pages)
    // 4. Create 11th page, should cause eviction using LRU
    // Since we unpinned p1 first, then p2..p10. 
    // LRU order depends on unpin order. P1 was unpinned recently in step 2.
    // Wait, step 2 unpinned p1. Then loop unpinned p2..p10.
    // So LRU list front should be p1? No.
    // Unpin adds to BACK of list (MRU). 
    // p1 unpin -> [p1]
    // loop unpins -> [p1, p2, p3... p10]
    // So p1 is LRU victim candidate? Yes.
    
    page_id_t p11_id;
    Page* p11 = bpm->new_page(&p11_id);
    assert(p11 != nullptr);
    // p1 should have been evicted and written to disk (since dirty)
    
    // 5. Fetch p1 again. Should read from disk.
    Page* p1_read = bpm->fetch_page(p1_id);
    assert(p1_read != nullptr);
    assert(strcmp(p1_read->get_data(), "Hello BufferPool") == 0);
    
    bpm->unpin_page(p11_id, false);
    bpm->unpin_page(p1_id, false);

    delete bpm;
    delete pm;
    std::remove(db_file.c_str());
    std::cout << "test_buffer_pool passed" << std::endl;
}

int main() {
    test_buffer_pool();
    return 0;
}
