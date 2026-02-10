/**
 * @file wal.cpp
 * @brief WAL 實現文件
 */

#include "wal.h"
#include <iostream>
#include <cstring>
#include <sys/stat.h>

#ifdef _WIN32
#include <direct.h>
#define mkdir(path, mode) _mkdir(path)
#else
#include <sys/types.h>
#endif

namespace kvengine {

// 簡單的 CRC32 實現
static const uint32_t CRC32_TABLE[256] = {
    0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
    0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
    0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
    0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
    0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
    0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
    0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
    0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
    0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
    0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
    0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106,
    0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
    0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
    0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
    0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
    0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
    0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
    0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
    0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
    0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
    0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
    0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
    0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
    0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
    0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
    0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
    0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
    0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
    0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
    0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
    0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
    0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
    0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
    0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
    0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
    0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
    0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
    0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
    0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
    0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
    0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
    0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
    0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

static uint32_t crc32(const uint8_t* data, size_t length) {
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < length; ++i) {
        crc = CRC32_TABLE[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
    }
    return crc ^ 0xFFFFFFFF;
}

WAL::WAL(const std::string& log_dir)
    : log_dir_(log_dir),
      current_lsn_(0),
      is_open_(false) {
}

WAL::~WAL() {
    close();
}

bool WAL::initialize() {
    std::cerr << "WAL::initialize called for " << log_dir_ << std::endl;
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 創建日誌目錄
    struct stat info;
    if (stat(log_dir_.c_str(), &info) != 0) {
        std::cerr << "Log dir " << log_dir_ << " does not exist, creating..." << std::endl;
        if (mkdir(log_dir_.c_str(), 0755) != 0) {
            std::cerr << "Failed to create log directory: " << log_dir_ << std::endl;
            return false;
        }
    }
    
    log_file_path_ = get_log_file_path();
    
    // 打開日誌文件（追加模式）
    log_stream_.open(log_file_path_, 
                     std::ios::binary | std::ios::in | std::ios::out | std::ios::app);
    
    if (!log_stream_.is_open()) {
        // 如果文件不存在，創建新文件
        log_stream_.open(log_file_path_, 
                        std::ios::binary | std::ios::out | std::ios::trunc);
        log_stream_.close();
        log_stream_.open(log_file_path_, 
                        std::ios::binary | std::ios::in | std::ios::out | std::ios::app);
    }
    
    if (!log_stream_.is_open()) {
        std::cerr << "Failed to open log file: " << log_file_path_ << std::endl;
        return false;
    }
    std::cout << "WAL opened file: " << log_file_path_ << std::endl;
    
    // 讀取現有日誌以確定當前 LSN
    log_stream_.seekg(0, std::ios::end);
    size_t file_size = log_stream_.tellg();
    
    if (file_size > 0) {
        // 讀取所有記錄以找到最大 LSN
        auto records = read_all_internal();
        for (const auto& record : records) {
            if (record.lsn > current_lsn_) {
                current_lsn_ = record.lsn;
            }
        }
    }
    
    is_open_ = true;
    return true;
}

uint64_t WAL::append(LogRecord& record) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!is_open_) {
        return 0;
    }
    
    // 分配 LSN
    record.lsn = ++current_lsn_;
    
    // 計算校驗和
    record.checksum = calculate_checksum(record);
    
    // 序列化記錄
    auto data = serialize_record(record);
    
    // 寫入文件
    log_stream_.write(reinterpret_cast<const char*>(data.data()), data.size());
    if (!log_stream_) {
        std::cerr << "WAL write failed for LSN " << record.lsn << std::endl;
        log_stream_.clear();
    }
    
    // 添加到緩衝區
    buffer_.push_back(record);
    
    // 如果緩衝區滿了，自動刷新
    if (buffer_.size() >= BUFFER_SIZE) {
        flush();
    }
    
    return record.lsn;
}

bool WAL::flush() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!is_open_) {
        return false;
    }
    flush_internal();
    return true;
}

void WAL::flush_internal() {
    log_stream_.flush();
    buffer_.clear();
}

uint64_t WAL::get_last_lsn() const {
    return current_lsn_;
}

std::vector<LogRecord> WAL::read_from(uint64_t start_lsn) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<LogRecord> records;
    if (!is_open_) {
        return records;
    }
    
    auto all = read_all_internal();
    for (const auto& record : all) {
        if (record.lsn >= start_lsn) {
            records.push_back(record);
        }
    }
    
    return records;
}

std::vector<LogRecord> WAL::read_all_internal() {
    std::vector<LogRecord> records;
    
    // 清除流狀態（重要：EOF 可能導致 seek 失敗）
    log_stream_.clear();
    
    // 讀取整個文件
    log_stream_.seekg(0, std::ios::end);
    std::streamsize file_size = log_stream_.tellg();
    log_stream_.seekg(0, std::ios::beg);
    
    if (file_size <= 0) {
        return records;
    }
    
    std::vector<uint8_t> data(static_cast<size_t>(file_size));
    log_stream_.read(reinterpret_cast<char*>(data.data()), file_size);
    
    // 反序列化所有記錄
    size_t offset = 0;
    while (offset < data.size()) {
        try {
            LogRecord record = deserialize_record(data, offset);
            
            // 驗證校驗和
            uint32_t expected_checksum = calculate_checksum(record);
            if (record.checksum != expected_checksum) {
                std::cerr << "Checksum mismatch for LSN " << record.lsn << std::endl;
                break;
            }
            
            records.push_back(record);
        } catch (...) {
            break;
        }
    }
    
    return records;
}

bool WAL::truncate(uint64_t lsn) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!is_open_) {
        return false;
    }
    
    // 讀取所有記錄
    auto all_records = read_all_internal();
    
    // 關閉當前文件
    log_stream_.close();
    
    // 重新創建文件
    log_stream_.open(log_file_path_, 
                     std::ios::binary | std::ios::out | std::ios::trunc);
    
    // 寫入 LSN >= lsn 的記錄
    for (const auto& record : all_records) {
        if (record.lsn >= lsn) {
            auto data = serialize_record(record);
            log_stream_.write(reinterpret_cast<const char*>(data.data()), data.size());
        }
    }
    
    log_stream_.close();
    
    // 重新打開
    log_stream_.open(log_file_path_, 
                     std::ios::binary | std::ios::in | std::ios::out | std::ios::app);
    
    return log_stream_.is_open();
}

void WAL::close() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (is_open_) {
        flush_internal();
        log_stream_.close();
        is_open_ = false;
    }
}

uint32_t WAL::calculate_checksum(const LogRecord& record) {
    std::vector<uint8_t> data;
    
    // 包含類型、事務 ID、LSN、鍵和值
    data.push_back(static_cast<uint8_t>(record.type));
    
    for (int i = 0; i < 8; ++i) {
        data.push_back((record.txn_id >> (i * 8)) & 0xFF);
    }
    
    for (int i = 0; i < 8; ++i) {
        data.push_back((record.lsn >> (i * 8)) & 0xFF);
    }
    
    data.insert(data.end(), record.key.begin(), record.key.end());
    data.insert(data.end(), record.value.begin(), record.value.end());
    
    return crc32(data.data(), data.size());
}

std::vector<uint8_t> WAL::serialize_record(const LogRecord& record) {
    std::vector<uint8_t> data;
    
    // Type (1 byte)
    data.push_back(static_cast<uint8_t>(record.type));
    
    // TxnID (8 bytes)
    for (int i = 0; i < 8; ++i) {
        data.push_back((record.txn_id >> (i * 8)) & 0xFF);
    }
    
    // LSN (8 bytes)
    for (int i = 0; i < 8; ++i) {
        data.push_back((record.lsn >> (i * 8)) & 0xFF);
    }
    
    // KeyLen (4 bytes)
    uint32_t key_len = static_cast<uint32_t>(record.key.size());
    for (int i = 0; i < 4; ++i) {
        data.push_back((key_len >> (i * 8)) & 0xFF);
    }
    
    // Key
    data.insert(data.end(), record.key.begin(), record.key.end());
    
    // ValueLen (4 bytes)
    uint32_t value_len = static_cast<uint32_t>(record.value.size());
    for (int i = 0; i < 4; ++i) {
        data.push_back((value_len >> (i * 8)) & 0xFF);
    }
    
    // Value
    data.insert(data.end(), record.value.begin(), record.value.end());
    
    // Checksum (4 bytes)
    for (int i = 0; i < 4; ++i) {
        data.push_back((record.checksum >> (i * 8)) & 0xFF);
    }
    
    return data;
}

LogRecord WAL::deserialize_record(const std::vector<uint8_t>& data, size_t& offset) {
    LogRecord record;
    
    if (offset >= data.size()) {
        throw std::runtime_error("Offset out of range");
    }
    
    // Type
    record.type = static_cast<LogRecordType>(data[offset++]);
    
    // TxnID
    record.txn_id = 0;
    for (int i = 0; i < 8; ++i) {
        record.txn_id |= (static_cast<uint64_t>(data[offset++]) << (i * 8));
    }
    
    // LSN
    record.lsn = 0;
    for (int i = 0; i < 8; ++i) {
        record.lsn |= (static_cast<uint64_t>(data[offset++]) << (i * 8));
    }
    
    // KeyLen
    uint32_t key_len = 0;
    for (int i = 0; i < 4; ++i) {
        key_len |= (static_cast<uint32_t>(data[offset++]) << (i * 8));
    }
    
    // Key
    record.key.assign(data.begin() + offset, data.begin() + offset + key_len);
    offset += key_len;
    
    // ValueLen
    uint32_t value_len = 0;
    for (int i = 0; i < 4; ++i) {
        value_len |= (static_cast<uint32_t>(data[offset++]) << (i * 8));
    }
    
    // Value
    record.value.assign(data.begin() + offset, data.begin() + offset + value_len);
    offset += value_len;
    
    // Checksum
    record.checksum = 0;
    for (int i = 0; i < 4; ++i) {
        record.checksum |= (static_cast<uint32_t>(data[offset++]) << (i * 8));
    }
    
    return record;
}

std::string WAL::get_log_file_path() const {
#ifdef _WIN32
    return log_dir_ + "\\wal.log";
#else
    return log_dir_ + "/wal.log";
#endif
}

} // namespace kvengine
