#include "../include/kvengine/kv_engine.h"
#include "storage_engine.h"
#include "hash_index.h"
#include "memory_manager.h"
#include "wal.h"
#include "lock_manager.h"
#include "transaction_manager.h"
#include "checkpoint_manager.h"
#include "recovery_manager.h"
#include <iostream>

namespace kvengine {

// Private implementation (Pimpl idiom)
class KvEngine::Impl {
public:
    explicit Impl(const std::string& data_dir)
        : data_dir_(data_dir),
          storage_(data_dir),
          wal_(data_dir),
          lock_mgr_(),
          txn_mgr_(&wal_, &lock_mgr_, &storage_),
          checkpoint_mgr_(&wal_, &txn_mgr_, &storage_),
          recovery_mgr_(&wal_, &storage_),
          is_open_(false) {
    }
    
    ~Impl() {
        if (is_open_) {
            close();
        }
    }
    
    bool open() {
        if (is_open_) {
            return true;
        }
        
        if (!storage_.initialize()) {
            std::cerr << "Failed to initialize storage engine" << std::endl;
            return false;
        }
        
        if (!wal_.initialize()) {
            std::cerr << "Failed to initialize WAL" << std::endl;
            return false;
        }
        
        // Perform recovery
        if (!recovery_mgr_.recover()) {
            std::cerr << "Recovery failed" << std::endl;
            return false;
        }
        
        // Build index from loaded data
        rebuild_index();
        
        is_open_ = true;
        return true;
    }
    
    void close() {
        if (!is_open_) {
            return;
        }
        
        // Create checkpoint before closing
        checkpoint_mgr_.create_checkpoint();
        
        // Flush data to disk
        storage_.flush();
        wal_.close();
        is_open_ = false;
    }
    
    bool is_open() const {
        return is_open_;
    }
    
    bool put(const std::string& key, const std::string& value) {
        if (!is_open_) {
            return false;
        }
        
        // 使用事務進行更新
        Transaction* txn = txn_mgr_.begin();
        if (!txn) {
            return false;
        }
        
        if (!txn_mgr_.put(txn, key, value)) {
            txn_mgr_.rollback(txn);
            return false;
        }
        
        if (!txn_mgr_.commit(txn)) {
            txn_mgr_.rollback(txn);
            return false;
        }
        
        // Update index (in memory structure)
        index_.insert(key, 0); 
        
        // Update statistics
        stats_.total_writes++;
        
        return true;
    }
    
    std::string get(const std::string& key) {
        std::string value;
        get(key, value);
        return value;
    }
    
    Status get(const std::string& key, std::string& value) {
        if (!is_open_) {
            return Status::IOError("Engine not open");
        }
        
        stats_.total_reads++;
        
        if (storage_.get(key, value)) {
            return Status::OK();
        }
        
        return Status::NotFound();
    }
    
    bool remove(const std::string& key) {
        if (!is_open_) {
            return false;
        }
        
        // 使用事務進行刪除
        Transaction* txn = txn_mgr_.begin();
        if (!txn) {
            return false;
        }
        
        if (!txn_mgr_.remove(txn, key)) {
            txn_mgr_.rollback(txn);
            return false;
        }
        
        if (!txn_mgr_.commit(txn)) {
            txn_mgr_.rollback(txn);
            return false;
        }
        
        // Remove from index
        index_.remove(key);
        
        return true;
    }
    
    bool exists(const std::string& key) {
        if (!is_open_) {
            return false;
        }
        
        return index_.exists(key);
    }
    
    bool batch_put(const std::map<std::string, std::string>& batch) {
        if (!is_open_) {
            return false;
        }
        
        // 開啟一個新事務處理所有批量操作
        Transaction* txn = txn_mgr_.begin();
        if (!txn) {
            return false;
        }
        
        for (const auto& pair : batch) {
            if (!txn_mgr_.put(txn, pair.first, pair.second)) {
                txn_mgr_.rollback(txn);
                return false;
            }
        }
        
        // 提交事務
        return txn_mgr_.commit(txn);
    }
    
    std::unique_ptr<Iterator> scan(const std::string& prefix) {
        if (!is_open_) {
            return nullptr;
        }
        
        return std::unique_ptr<Iterator>(
            new MapIterator(storage_.get_all_data(), prefix)
        );
    }
    
    Statistics get_statistics() const {
        Statistics stats = stats_;
        stats.total_keys = index_.size();
        stats.memory_used = storage_.memory_usage();
        return stats;
    }
    
    bool flush() {
        if (!is_open_) {
            return false;
        }
        
        checkpoint_mgr_.create_checkpoint();
        return storage_.flush();
    }
    
    bool verify_integrity() {
        if (!is_open_) {
            return false;
        }
        
        // Simple integrity check: verify index matches storage
        auto keys = index_.get_all_keys();
        for (const auto& key : keys) {
            if (!storage_.exists(key)) {
                std::cerr << "Integrity error: key in index but not in storage: " 
                         << key << std::endl;
                return false;
            }
        }
        
        return true;
    }
    
private:
    void rebuild_index() {
        index_.clear();
        const auto& data = storage_.get_all_data();
        for (const auto& pair : data) {
            index_.insert(pair.first, 0);
        }
        stats_.total_keys = index_.size();
    }
    
    std::string data_dir_;
    StorageEngine storage_;
    WAL wal_;
    LockManager lock_mgr_;
    TransactionManager txn_mgr_;
    CheckpointManager checkpoint_mgr_;
    RecoveryManager recovery_mgr_;
    HashIndex index_;
    MemoryManager memory_;
    Statistics stats_;
    bool is_open_;
};

// KvEngine implementation

KvEngine::KvEngine(const std::string& data_dir)
    : impl_(new Impl(data_dir)) {
}

KvEngine::~KvEngine() {
}

bool KvEngine::open() {
    return impl_->open();
}

void KvEngine::close() {
    impl_->close();
}

bool KvEngine::is_open() const {
    return impl_->is_open();
}

bool KvEngine::put(const std::string& key, const std::string& value) {
    return impl_->put(key, value);
}

std::string KvEngine::get(const std::string& key) {
    return impl_->get(key);
}

Status KvEngine::get(const std::string& key, std::string& value) {
    return impl_->get(key, value);
}

bool KvEngine::remove(const std::string& key) {
    return impl_->remove(key);
}

bool KvEngine::exists(const std::string& key) {
    return impl_->exists(key);
}

bool KvEngine::batch_put(const std::map<std::string, std::string>& batch) {
    return impl_->batch_put(batch);
}

std::unique_ptr<Iterator> KvEngine::scan(const std::string& prefix) {
    return impl_->scan(prefix);
}

Statistics KvEngine::get_statistics() const {
    return impl_->get_statistics();
}

bool KvEngine::flush() {
    return impl_->flush();
}

bool KvEngine::verify_integrity() {
    return impl_->verify_integrity();
}

} // namespace kvengine
