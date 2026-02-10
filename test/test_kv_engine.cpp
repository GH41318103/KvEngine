/**
 * @file test_kv_engine.cpp
 * @brief KvEngine 單元測試
 * @details 測試所有核心功能的正確性
 */

#include <kvengine/kv_engine.h>
#include <iostream>
#include <cassert>
#include <string>

using namespace kvengine;

// 測試基本操作：put, get, exists, remove
// 測試基本操作：put, get, exists, remove
void test_basic_operations() {
    std::cout << "Testing basic operations..." << std::endl;
    
    KvEngine engine("./test_basic");
    if (!engine.open()) { std::cerr << "Open failed" << std::endl; abort(); }
    
    // 測試 put 和 get
    if (!engine.put("key1", "value1")) abort();
    if (engine.get("key1") != "value1") abort();
    
    // 測試 exists
    if (!engine.exists("key1")) abort();
    if (engine.exists("nonexistent")) abort();
    
    // 測試 remove
    if (!engine.remove("key1")) abort();
    if (engine.exists("key1")) abort();
    if (!engine.get("key1").empty()) abort();
    
    engine.close();
    std::cout << "  ✓ Basic operations test passed" << std::endl;
}

// 測試多個鍵的操作
void test_multiple_keys() {
    std::cout << "Testing multiple keys..." << std::endl;
    
    KvEngine engine("./test_multi");
    if (!engine.open()) abort();
    
    // 插入多個鍵
    for (int i = 0; i < 100; ++i) {
        std::string key = "key" + std::to_string(i);
        std::string value = "value" + std::to_string(i);
        if (!engine.put(key, value)) abort();
    }
    
    // 驗證所有鍵
    for (int i = 0; i < 100; ++i) {
        std::string key = "key" + std::to_string(i);
        std::string expected = "value" + std::to_string(i);
        if (engine.get(key) != expected) abort();
    }
    
    // 檢查統計信息
    Statistics stats = engine.get_statistics();
    if (stats.total_keys != 100) abort();
    
    engine.close();
    std::cout << "  ✓ Multiple keys test passed" << std::endl;
}

// 測試數據持久化
void test_persistence() {
    std::cout << "Testing persistence..." << std::endl;
    
    // 寫入數據
    {
        KvEngine engine("./test_persist");
        if (!engine.open()) abort();
        
        engine.put("persist1", "value1");
        engine.put("persist2", "value2");
        engine.put("persist3", "value3");
        
        if (!engine.flush()) std::cerr << "Flush failed" << std::endl;
        engine.close();
    }
    
    // 在新實例中讀取數據
    {
        KvEngine engine("./test_persist");
        if (!engine.open()) abort();
        
        if (engine.get("persist1") != "value1") abort();
        if (engine.get("persist2") != "value2") abort();
        if (engine.get("persist3") != "value3") abort();
        
        engine.close();
    }
    
    std::cout << "  ✓ Persistence test passed" << std::endl;
}

// 測試批量操作
void test_batch_operations() {
    std::cout << "Testing batch operations..." << std::endl;
    
    KvEngine engine("./test_batch");
    if (!engine.open()) abort();
    
    // 準備批量數據
    std::map<std::string, std::string> batch;
    for (int i = 0; i < 50; ++i) {
        batch["batch:" + std::to_string(i)] = "value" + std::to_string(i);
    }
    
    // 批量插入
    if (!engine.batch_put(batch)) abort();
    
    // 驗證
    for (const auto& pair : batch) {
        if (engine.get(pair.first) != pair.second) abort();
    }
    
    engine.close();
    std::cout << "  ✓ Batch operations test passed" << std::endl;
}

// 測試迭代器功能
void test_iterator() {
    std::cout << "Testing iterator..." << std::endl;
    
    KvEngine engine("./test_iter");
    if (!engine.open()) { std::cerr << "Open failed in iterator test" << std::endl; abort(); }
    
    // 插入測試數據
    if (!engine.put("user:1:name", "Alice")) abort();
    if (!engine.put("user:2:name", "Bob")) abort();
    if (!engine.put("user:3:name", "Charlie")) abort();
    if (!engine.put("config:db:host", "localhost")) abort();
    
    // 驗證數據已插入
    if (engine.get("user:1:name") != "Alice") abort();
    if (engine.get("config:db:host") != "localhost") abort();
    
    // 測試前綴掃描
    int count = 0;
    auto iterator = engine.scan("user:");
    if (!iterator) { std::cerr << "Scan returned nullptr" << std::endl; abort(); }
    
    while (iterator->valid()) {
        std::string key = iterator->key();
        if (key.substr(0, 5) != "user:") abort();
        count++;
        iterator->next();
    }
    if (count != 3) { std::cerr << "Count mismatch: " << count << std::endl; abort(); }
    
    // 測試全遍歷
    count = 0;
    auto all_iterator = engine.scan();
    if (!all_iterator) abort();
    
    while (all_iterator->valid()) {
        count++;
        all_iterator->next();
    }
    if (count < 4) abort();
    
    engine.close();
    std::cout << "  ✓ Iterator test passed" << std::endl;
}

// 測試邊界情況
void test_edge_cases() {
    std::cout << "Testing edge cases..." << std::endl;
    
    KvEngine engine("./test_edge");
    if (!engine.open()) abort();
    
    // 空鍵和空值
    if (!engine.put("", "empty_key")) abort();
    if (engine.get("") != "empty_key") abort();
    
    if (!engine.put("empty_value", "")) abort();
    if (engine.get("empty_value") != "") abort();
    
    // 大值
    std::string large_value(10000, 'x');
    if (!engine.put("large", large_value)) abort();
    if (engine.get("large") != large_value) abort();
    
    // 覆寫
    if (!engine.put("overwrite", "value1")) abort();
    if (engine.get("overwrite") != "value1") abort();
    if (!engine.put("overwrite", "value2")) abort();
    if (engine.get("overwrite") != "value2") abort();
    
    engine.close();
    std::cout << "  ✓ Edge cases test passed" << std::endl;
}

// 主測試函數
int main() {
    std::cout << "=== KvEngine Test Suite ===" << std::endl << std::endl;
    
    try {
        test_basic_operations();
        test_multiple_keys();
        test_persistence();
        test_batch_operations();
        test_iterator();
        test_edge_cases();
        
        std::cout << std::endl << "=== All tests passed! ===" << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Test failed with unknown exception" << std::endl;
        return 1;
    }
}
