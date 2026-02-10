/**
 * @file test_wal.cpp
 * @brief WAL 單元測試
 */

#include "../src/kvengine/wal.h"
#include <iostream>
#include <cassert>
#include <filesystem>

using namespace kvengine;

// 測試 WAL 初始化
void test_wal_initialize() {
    std::cout << "Testing WAL initialization..." << std::endl;
    
    WAL wal("./test_wal_data");
    assert(wal.initialize());
    assert(wal.get_last_lsn() == 0);
    
    wal.close();
    std::cout << "  ✓ WAL initialization test passed" << std::endl;
}

// 測試日誌追加和讀取
void test_wal_append_and_read() {
    std::cout << "Testing WAL append and read..." << std::endl;
    
    WAL wal("./test_wal_data");
    assert(wal.initialize());
    
    // 追加幾條記錄
    LogRecord rec1(LogRecordType::PUT, 1, "key1", "value1");
    LogRecord rec2(LogRecordType::PUT, 1, "key2", "value2");
    LogRecord rec3(LogRecordType::DELETE, 1, "key1");
    
    uint64_t lsn1 = wal.append(rec1);
    uint64_t lsn2 = wal.append(rec2);
    uint64_t lsn3 = wal.append(rec3);
    
    assert(lsn1 == 1);
    assert(lsn2 == 2);
    assert(lsn3 == 3);
    assert(wal.get_last_lsn() == 3);
    
    // 刷新到磁盤
    assert(wal.flush());
    
    // 讀取記錄
    auto records = wal.read_from(0);
    assert(records.size() == 3);
    assert(records[0].key == "key1");
    assert(records[0].value == "value1");
    assert(records[1].key == "key2");
    assert(records[2].type == LogRecordType::DELETE);
    
    wal.close();
    std::cout << "  ✓ WAL append and read test passed" << std::endl;
}

// 測試持久化
void test_wal_persistence() {
    std::cout << "Testing WAL persistence..." << std::endl;
    
    // 寫入數據
    {
        WAL wal("./test_wal_persist");
        assert(wal.initialize());
        
        LogRecord rec1(LogRecordType::BEGIN, 1, "");
        LogRecord rec2(LogRecordType::PUT, 1, "persist_key", "persist_value");
        LogRecord rec3(LogRecordType::COMMIT, 1, "");
        
        wal.append(rec1);
        wal.append(rec2);
        wal.append(rec3);
        wal.flush();
        wal.close();
    }
    
    // 重新打開並讀取
    {
        WAL wal("./test_wal_persist");
        assert(wal.initialize());
        
        assert(wal.get_last_lsn() == 3);
        
        auto records = wal.read_from(0);
        assert(records.size() == 3);
        assert(records[0].type == LogRecordType::BEGIN);
        assert(records[1].key == "persist_key");
        assert(records[1].value == "persist_value");
        assert(records[2].type == LogRecordType::COMMIT);
        
        wal.close();
    }
    
    std::cout << "  ✓ WAL persistence test passed" << std::endl;
}

// 測試日誌截斷
void test_wal_truncate() {
    std::cout << "Testing WAL truncate..." << std::endl;
    
    WAL wal("./test_wal_truncate");
    assert(wal.initialize());
    
    // 添加多條記錄
    for (int i = 1; i <= 10; ++i) {
        LogRecord rec(LogRecordType::PUT, 1, "key" + std::to_string(i), "value" + std::to_string(i));
        wal.append(rec);
    }
    wal.flush();
    
    // 截斷到 LSN 6
    assert(wal.truncate(6));
    
    // 驗證只剩下 LSN >= 6 的記錄
    auto records = wal.read_from(0);
    assert(records.size() == 5);  // LSN 6, 7, 8, 9, 10
    assert(records[0].lsn == 6);
    assert(records[4].lsn == 10);
    
    wal.close();
    std::cout << "  ✓ WAL truncate test passed" << std::endl;
}

// 測試校驗和驗證
void test_wal_checksum() {
    std::cout << "Testing WAL checksum..." << std::endl;
    
    WAL wal("./test_wal_checksum");
    assert(wal.initialize());
    
    LogRecord rec(LogRecordType::PUT, 1, "checksum_key", "checksum_value");
    wal.append(rec);
    wal.flush();
    
    // 讀取並驗證校驗和
    auto records = wal.read_from(0);
    assert(records.size() == 1);
    // 如果校驗和不匹配，read_from 會停止讀取
    
    wal.close();
    std::cout << "  ✓ WAL checksum test passed" << std::endl;
}

// 主測試函數
int main() {
    std::cout << "=== WAL Test Suite ===" << std::endl << std::endl;
    
    try {
        test_wal_initialize();
        test_wal_append_and_read();
        test_wal_persistence();
        test_wal_truncate();
        test_wal_checksum();
        
        std::cout << std::endl << "=== All WAL tests passed! ===" << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Test failed with unknown exception" << std::endl;
        return 1;
    }
}
