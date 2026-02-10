#include "storage_engine.h"
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <iostream>
#include <vector>

#ifdef _WIN32
#include <direct.h>
#define mkdir(path, mode) _mkdir(path)
#else
#include <sys/types.h>
#endif

namespace kvengine {

StorageEngine::StorageEngine(const std::string& data_dir)
    : data_dir_(data_dir) {
    data_file_ = get_data_file_path();
}

StorageEngine::~StorageEngine() {
    flush();
}

bool StorageEngine::initialize() {
    // Create data directory if it doesn't exist
    struct stat info;
    if (stat(data_dir_.c_str(), &info) != 0) {
        // Directory doesn't exist, create it
        if (mkdir(data_dir_.c_str(), 0755) != 0) {
            std::cerr << "Failed to create data directory: " << data_dir_ << std::endl;
            return false;
        }
    }
    
    // Try to load existing data
    return load();
}

bool StorageEngine::put(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(mutex_);
    data_[key] = value;
    return true;
}

bool StorageEngine::get(const std::string& key, std::string& value) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = data_.find(key);
    if (it != data_.end()) {
        value = it->second;
        return true;
    }
    return false;
}

bool StorageEngine::remove(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    return data_.erase(key) > 0;
}

bool StorageEngine::exists(const std::string& key) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return data_.find(key) != data_.end();
}

const std::map<std::string, std::string>& StorageEngine::get_all_data() const {
    return data_;
}

bool StorageEngine::flush() {
    std::lock_guard<std::mutex> lock(mutex_);
    return serialize_to_file(data_file_);
}

bool StorageEngine::load() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if file exists
    std::ifstream test(data_file_);
    if (!test.good()) {
        // File doesn't exist, start with empty data
        return true;
    }
    test.close();
    
    return deserialize_from_file(data_file_);
}

size_t StorageEngine::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return data_.size();
}

size_t StorageEngine::memory_usage() const {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t total = 0;
    for (const auto& pair : data_) {
        total += pair.first.size() + pair.second.size();
    }
    return total;
}

bool StorageEngine::serialize_to_file(const std::string& filename) {
    std::ofstream ofs(filename, std::ios::binary);
    if (!ofs.is_open()) {
        std::cerr << "Failed to open file for writing: " << filename << std::endl;
        return false;
    }
    
    // 定義緩衝區大小 (8KB)
    const size_t BUFFER_SIZE = 8192;
    std::vector<char> buffer;
    buffer.reserve(BUFFER_SIZE);
    
    // 寫入緩衝區的 lambda
    auto write_to_buffer = [&](const char* data, size_t size) {
        if (buffer.size() + size > BUFFER_SIZE) {
            ofs.write(buffer.data(), buffer.size());
            buffer.clear();
        }
        
        if (size > BUFFER_SIZE) {
            // 如果單個數據超過緩衝區大小，直接寫入
            ofs.write(data, size);
        } else {
            buffer.insert(buffer.end(), data, data + size);
        }
    };
    
    // Write number of entries
    uint64_t num_entries = data_.size();
    write_to_buffer(reinterpret_cast<const char*>(&num_entries), sizeof(num_entries));
    
    // Write each key-value pair
    for (const auto& pair : data_) {
        // Write key length and key
        uint32_t key_len = static_cast<uint32_t>(pair.first.size());
        write_to_buffer(reinterpret_cast<const char*>(&key_len), sizeof(key_len));
        write_to_buffer(pair.first.data(), key_len);
        
        // Write value length and value
        uint32_t value_len = static_cast<uint32_t>(pair.second.size());
        write_to_buffer(reinterpret_cast<const char*>(&value_len), sizeof(value_len));
        write_to_buffer(pair.second.data(), value_len);
    }
    
    // 寫入剩餘緩衝區
    if (!buffer.empty()) {
        ofs.write(buffer.data(), buffer.size());
    }
    
    ofs.close();
    return true;
}

bool StorageEngine::deserialize_from_file(const std::string& filename) {
    std::ifstream ifs(filename, std::ios::binary);
    if (!ifs.is_open()) {
        std::cerr << "Failed to open file for reading: " << filename << std::endl;
        return false;
    }
    
    // Read number of entries
    uint64_t num_entries = 0;
    ifs.read(reinterpret_cast<char*>(&num_entries), sizeof(num_entries));
    
    if (ifs.fail()) {
        std::cerr << "Failed to read number of entries" << std::endl;
        return false;
    }
    
    // Clear existing data
    data_.clear();
    
    // Read each key-value pair
    for (uint64_t i = 0; i < num_entries; ++i) {
        // Read key
        uint32_t key_len = 0;
        ifs.read(reinterpret_cast<char*>(&key_len), sizeof(key_len));
        
        std::string key(key_len, '\0');
        ifs.read(&key[0], key_len);
        
        // Read value
        uint32_t value_len = 0;
        ifs.read(reinterpret_cast<char*>(&value_len), sizeof(value_len));
        
        std::string value(value_len, '\0');
        ifs.read(&value[0], value_len);
        
        if (ifs.fail()) {
            std::cerr << "Failed to read entry " << i << std::endl;
            return false;
        }
        
        data_[key] = value;
    }
    
    ifs.close();
    return true;
}

std::string StorageEngine::get_data_file_path() const {
#ifdef _WIN32
    return data_dir_ + "\\kvengine.dat";
#else
    return data_dir_ + "/kvengine.dat";
#endif
}

} // namespace kvengine
