#  KvEngine 進階指令與開發教學 (Advanced Usage Guide)

本文件旨在為開發者提供 **KvEngine** 目前已實現的進階功能教學，包含批量操作、數據搜尋、系統監控以及底層事務機制的詳細說明。

---

## 1.  原子性批量操作 (Atomic Batch Put)

在處理大量數據寫入時，頻繁調用單次 `put()` 會導致日誌頻繁刷盤及多次索引更新。推薦使用 `batch_put`，它在內部會自動開啟一個事務。

### 為什麼使用 Batch Put？
- **原子性**：整個批次要麼全部成功，要麼全部失敗。
- **高性能**：減少了日誌寫入次數，相比單次寫入提升約 10 倍效能。

### 代碼範例
```cpp
#include <kvengine/kv_engine.h>
#include <map>

// ... 初始化 engine ...

std::map<std::string, std::string> updates = {
    {"config:timeout", "300"},
    {"config:retries", "5"},
    {"config:mode", "async"}
};

if (engine.batch_put(updates)) {
    // 以上三個鍵值對將以原子方式寫入
}
```

---

## 2.  前綴搜尋與高效掃描 (Prefix Scanning)

KvEngine 支援基於哈希與前綴索引的高效搜尋。透過 `scan()` 方法，你可以遍歷特定命名空間下的所有數據。

### 使用場景：遍歷用戶信息
```cpp
// 掃描所有以 "user:1001:" 開頭的鍵（如 "user:1001:name", "user:1001:email"）
auto it = engine.scan("user:1001:");

while (it->valid()) {
    std::cout << "Key: " << it->key() << " | Value: " << it->value() << std::endl;
    it->next(); // 移動到下一個符合條件的鍵
}
```

---

## 3.  系統狀態監控 (Statistics)

透過 `get_statistics()`，開發者可以即時獲取引擎的運行狀態，這對於性能調優至關重要。

### 指標說明
```cpp
kvengine::Statistics stats = engine.get_statistics();

std::cout << "總鍵值數: " << stats.total_keys << std::endl;
std::cout << "當前內存佔用: " << stats.memory_used / 1024 << " KB" << std::endl;
std::cout << "累計讀取次數: " << stats.total_reads << std::endl;
std::cout << "累計寫入次數: " << stats.total_writes << std::endl;
```

---

## 4.  Redis 兼容指令進階用法

啟動 `./kv_server` 後，你可以使用標準 Redis 工具鏈進行操作。

### 批量刪除與模糊搜尋
*   **搜尋所有鍵**：`KEYS *`
*   **按前綴搜尋**：`KEYS user:*`
*   **刪除多個鍵**：`DEL key1 key2 key3`

---

## 5.  維護與完整性驗證 (Maintenance)

對於關鍵應用，你可以手動觸發檢查點或驗證存儲文件是否損壞。

```cpp
// 1. 強制將數據刷入磁盤並建立 Checkpoint（回收舊日誌）
engine.flush();

// 2. 驗證內存索引與底層磁盤數據是否一致
if (engine.verify_integrity()) {
    std::cout << "數據完整性驗證通過！" << std::endl;
}
```

---

## 6.  故障恢復機制 (WAL & Recovery)

KvEngine 不需要手動「修復」數據。當系統崩潰重啟後，`open()` 方法會自動觸發以下流程：
1.  **Analysis**: 分析 WAL 日誌，找出未完成的事務。
2.  **Redo**: 重放已提交的事務數據。
3.  **Undo**: 回滾未提交的事務。

**手動測試方法**：在 `put` 過程中強行關閉進程，重新打開引擎後使用 `get` 驗證數據是否存在。

---

## 7.  底層核心接口 (For Core Developers)

如果你需要更細粒度的併發控制，可以直接使用內部的 `TransactionManager`。

> [!WARNING]
> 直接操作核心組件需要自行管理鎖與內存排他性，除非是開發存儲引擎插件，否則建議使用 `KvEngine` 公開 API。

```cpp
// 高級：直接與 TransactionManager 交互（詳見 test_transaction.cpp）
Transaction* txn = txn_mgr.begin();
txn_mgr.put(txn, "key", "value");
txn_mgr.commit(txn);
```

---

##  性能調優建議

1.  **編譯選項**：生產環境請務必使用 `-DCMAKE_BUILD_TYPE=Release`，性能差異可達 5-10 倍。
2.  **磁盤效能**：在支持 `fsync` 的系統上，將 `data_dir` 置於 SSD 盤可顯著提升寫入吞吐量。
3.  **內存管理**：目前的存儲引擎基於頁面管理，通過調整 `BufferPoolSize`（未來版本開放配置）可控制內存上限。
