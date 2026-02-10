#include <kvengine/storage/b_plus_tree_leaf_page.h>
#include <kvengine/storage/b_plus_tree_internal_page.h>
#include <iostream>
#include <cassert>
#include <cstring>

using namespace kvengine;

// Simple Integer Comparator
struct IntComparator {
    int operator()(const int64_t &lhs, const int64_t &rhs) const {
        if (lhs < rhs) return -1;
        if (lhs > rhs) return 1;
        return 0;
    }
};

void test_leaf_page() {
    // Allocate raw memory for page
    char buf[PAGE_SIZE]; // 4KB
    memset(buf, 0, PAGE_SIZE);
    
    // Cast to LeafPage
    auto *leaf = reinterpret_cast<BPlusTreeLeafPage<int64_t, int64_t, IntComparator>*>(buf);
    
    leaf->init(1, INVALID_PAGE_ID, 100); // Max size 100
    assert(leaf->is_leaf_page());
    assert(leaf->get_size() == 0);
    
    IntComparator cmp;
    
    assert(leaf->insert(10, 100, cmp));
    assert(leaf->insert(5, 50, cmp));  // Should insert before 10
    assert(leaf->insert(20, 200, cmp));
    
    assert(leaf->get_size() == 3);
    assert(leaf->key_at(0) == 5);
    assert(leaf->key_at(1) == 10);
    assert(leaf->key_at(2) == 20);
    
    int64_t val;
    assert(leaf->lookup(5, val, cmp));
    assert(val == 50);
    
    assert(!leaf->lookup(99, val, cmp));
    
    std::cout << "test_leaf_page passed" << std::endl;
}

int main() {
    test_leaf_page();
    return 0;
}
