/**
 * @file wal.h
 * @brief Write-Ahead Logging (WAL) 實現
 * @details 提供預寫日誌功能，確保數據持久性和崩潰恢復
 */

#ifndef KVENGINE_WAL_H
#define KVENGINE_WAL_H

#include "../include/kvengine/types.h"
#include <string>
#include <vector>
#include <fstream>
#include <mutex>
#include <cstdint>
#include <atomic>

namespace kvengine {

/**
 * @enum LogRecordType
 * @brief 日誌記錄類型
 */
enum class LogRecordType : uint8_t {
    PUT = 1,           // 插入/更新操作
    DELETE = 2,        // 刪除操作
    BEGIN = 3,         // 開始事務
    COMMIT = 4,        // 提交事務
    ROLLBACK = 5,      // 回滾事務
    CHECKPOINT = 6     // 檢查點
};

/**
 * @struct LogRecord
 * @brief 日誌記錄結構
 */
struct LogRecord {
    LogRecordType type;      // 記錄類型
    uint64_t txn_id;         // 事務 ID
    uint64_t lsn;            // 日誌序列號 (Log Sequence Number)
    std::string key;         // 鍵
    std::string value;       // 值（DELETE 時為空）
    uint32_t checksum;       // CRC32 校驗和
    
    LogRecord() 
        : type(LogRecordType::PUT), txn_id(0), lsn(0), checksum(0) {}
    
    LogRecord(LogRecordType t, uint64_t tid, const std::string& k, const std::string& v = "")
        : type(t), txn_id(tid), lsn(0), key(k), value(v), checksum(0) {}
};

/**
 * @class WAL
 * @brief 預寫日誌管理器
 * @details 負責日誌的寫入、刷新和讀取
 */
class WAL {
public:
    /**
     * @brief 構造函數
     * @param log_dir 日誌目錄路徑
     */
    explicit WAL(const std::string& log_dir);
    
    /**
     * @brief 析構函數
     */
    ~WAL();
    
    /**
     * @brief 初始化 WAL
     * @return 成功返回 true
     */
    bool initialize();
    
    /**
     * @brief 追加日誌記錄
     * @param record 日誌記錄
     * @return 分配的 LSN
     */
    uint64_t append(LogRecord& record);
    
    /**
     * @brief 刷新日誌到磁盤
     * @return 成功返回 true
     */
    bool flush();
    
    /**
     * @brief 獲取最後的 LSN
     * @return 最後的 LSN
     */
    uint64_t get_last_lsn() const;
    
    /**
     * @brief 從指定 LSN 開始讀取日誌
     * @param start_lsn 起始 LSN
     * @return 日誌記錄列表
     */
    std::vector<LogRecord> read_from(uint64_t start_lsn);
    
    /**
     * @brief 截斷日誌（刪除指定 LSN 之前的日誌）
     * @param lsn 截斷點 LSN
     * @return 成功返回 true
     */
    bool truncate(uint64_t lsn);
    
    /**
     * @brief 關閉 WAL
     */
    void close();
    
private:
    std::string log_dir_;              // 日誌目錄
    std::string log_file_path_;        // 日誌文件路徑
    std::fstream log_stream_;          // 日誌文件流
    std::atomic<uint64_t> current_lsn_;// 當前 LSN（原子操作）
    std::mutex mutex_;                 // 線程安全鎖
    bool is_open_;                     // 是否已打開
    
    // 日誌緩衝
    std::vector<LogRecord> buffer_;    // 緩衝區
    static const size_t BUFFER_SIZE = 100;  // 緩衝區大小
    
    /**
     * @brief 計算 CRC32 校驗和
     * @param record 日誌記錄
     * @return 校驗和
     */
    uint32_t calculate_checksum(const LogRecord& record);
    
    /**
     * @brief 序列化日誌記錄
     * @param record 日誌記錄
     * @return 序列化後的字節數據
     */
    std::vector<uint8_t> serialize_record(const LogRecord& record);
    
    /**
     * @brief 反序列化日誌記錄
     * @param data 字節數據
     * @param offset 偏移量（輸入輸出參數）
     * @return 日誌記錄
     */
    LogRecord deserialize_record(const std::vector<uint8_t>& data, size_t& offset);
    
    /**
     * @brief 獲取日誌文件路徑
     * @return 文件路徑
     */
    std::string get_log_file_path() const;

    /**
     * @brief 內部刷新邏輯（不加鎖）
     */
    void flush_internal();

    /**
     * @brief 內部讀取邏輯（不加鎖）
     */
    std::vector<LogRecord> read_all_internal();
};

} // namespace kvengine

#endif // KVENGINE_WAL_H
