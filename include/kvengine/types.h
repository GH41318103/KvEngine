/**
 * @file types.h
 * @brief KvEngine 核心類型定義
 * @details 定義了狀態碼、切片類、統計信息和異常類型
 */

#ifndef KVENGINE_TYPES_H
#define KVENGINE_TYPES_H

#include <string>
#include <cstdint>
#include <stdexcept>

namespace kvengine {

// 操作狀態碼枚舉
enum class StatusCode {
    OK = 0,              // 操作成功
    NOT_FOUND = 1,       // 鍵未找到
    IO_ERROR = 2,        // I/O 錯誤
    CORRUPTION = 3,      // 數據損壞
    NOT_SUPPORTED = 4,   // 操作不支持
    INVALID_ARGUMENT = 5,// 無效參數
    ALREADY_EXISTS = 6   // 鍵已存在
};

// 操作結果狀態類
class Status {
public:
    // 默認構造函數 - 創建成功狀態
    Status() : code_(StatusCode::OK), message_("") {}
    
    // 帶狀態碼和消息的構造函數
    explicit Status(StatusCode code, const std::string& message = "")
        : code_(code), message_(message) {}
    
    // 檢查操作是否成功
    bool ok() const { return code_ == StatusCode::OK; }
    
    // 獲取狀態碼
    StatusCode code() const { return code_; }
    
    // 獲取錯誤消息
    const std::string& message() const { return message_; }
    
    // 工廠方法 - 創建各種狀態
    static Status OK() { return Status(); }
    
    static Status NotFound(const std::string& msg = "Key not found") {
        return Status(StatusCode::NOT_FOUND, msg);
    }
    
    static Status IOError(const std::string& msg = "IO error") {
        return Status(StatusCode::IO_ERROR, msg);
    }
    
    static Status Corruption(const std::string& msg = "Data corruption") {
        return Status(StatusCode::CORRUPTION, msg);
    }
    
    static Status InvalidArgument(const std::string& msg = "Invalid argument") {
        return Status(StatusCode::INVALID_ARGUMENT, msg);
    }

    bool is_not_found() const { return code_ == StatusCode::NOT_FOUND; }

    std::string to_string() const {
        if (code_ == StatusCode::OK) return "OK";
        if (!message_.empty()) return message_;
        switch (code_) {
            case StatusCode::NOT_FOUND: return "NotFound";
            case StatusCode::IO_ERROR: return "IOError";
            case StatusCode::CORRUPTION: return "Corruption";
            case StatusCode::NOT_SUPPORTED: return "NotSupported";
            case StatusCode::INVALID_ARGUMENT: return "InvalidArgument";
            case StatusCode::ALREADY_EXISTS: return "AlreadyExists";
            default: return "UnknownError";
        }
    }
    
private:
    StatusCode code_;      // 狀態碼
    std::string message_;  // 錯誤消息
};

/**
 * @class Slice
 * @brief 輕量級字符串視圖類（類似 C++17 的 std::string_view）
 * @details 提供對字符串的非擁有引用，避免不必要的拷貝
 */
class Slice {
public:
    // 默認構造函數
    Slice() : data_(""), size_(0) {}
    
    // 從指針和大小構造
    Slice(const char* data, size_t size) : data_(data), size_(size) {}
    
    // 從 std::string 構造
    Slice(const std::string& s) : data_(s.data()), size_(s.size()) {}
    
    // 從 C 字符串構造
    Slice(const char* s) : data_(s), size_(strlen(s)) {}
    
    // 獲取數據指針
    const char* data() const { return data_; }
    
    // 獲取大小
    size_t size() const { return size_; }
    
    // 檢查是否為空
    bool empty() const { return size_ == 0; }
    
    // 轉換為 std::string
    std::string to_string() const { return std::string(data_, size_); }
    
    // 下標訪問
    char operator[](size_t n) const { return data_[n]; }
    
    // 相等比較
    bool operator==(const Slice& other) const {
        return (size_ == other.size_) &&
               (memcmp(data_, other.data_, size_) == 0);
    }
    
    // 不等比較
    bool operator!=(const Slice& other) const {
        return !(*this == other);
    }
    
private:
    const char* data_;  // 數據指針（不擁有）
    size_t size_;       // 數據大小
};

/**
 * @struct Statistics
 * @brief 引擎統計信息結構
 */
struct Statistics {
    uint64_t total_keys = 0;      // 總鍵數
    uint64_t memory_used = 0;     // 內存使用量（字節）
    uint64_t cache_hit_rate = 0;  // 緩存命中率（百分比）
    uint64_t total_reads = 0;     // 總讀取次數
    uint64_t total_writes = 0;    // 總寫入次數
};

/**
 * @class KvEngineException
 * @brief KvEngine 異常類
 */
class KvEngineException : public std::runtime_error {
public:
    explicit KvEngineException(const std::string& message)
        : std::runtime_error(message) {}
};

} // namespace kvengine

#endif // KVENGINE_TYPES_H
