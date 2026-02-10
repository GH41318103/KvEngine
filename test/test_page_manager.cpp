#include <kvengine/storage/page_manager.h>
#include <kvengine/storage/page.h>
#include <iostream>
#include <cassert>
#include <cstring>
#include <cstdio>

using namespace kvengine;

void test_page_rw() {
    std::string db_file = "test_page_mgr.db";
    std::remove(db_file.c_str());

    PageManager pm(db_file);
    assert(pm.open());

    char data[PAGE_SIZE];
    memset(data, 'A', PAGE_SIZE);

    page_id_t p1 = pm.allocate_page();
    assert(p1 == 0);
    pm.write_page(p1, data);

    char read_buf[PAGE_SIZE];
    pm.read_page(p1, read_buf);
    
    assert(memcmp(data, read_buf, PAGE_SIZE) == 0);
    
    // allocate second page
    memset(data, 'B', PAGE_SIZE);
    page_id_t p2 = pm.allocate_page();
    assert(p2 == 1);
    pm.write_page(p2, data);

    pm.read_page(p2, read_buf);
    assert(read_buf[0] == 'B');
    
    // read back first
    pm.read_page(p1, read_buf);
    assert(read_buf[0] == 'A');

    pm.close();
    std::remove(db_file.c_str());
    std::cout << "test_page_rw passed" << std::endl;
}

int main() {
    test_page_rw();
    return 0;
}
