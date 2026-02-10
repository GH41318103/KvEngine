/**
 * @file transaction.cpp
 * @brief 事務實現文件
 */

#include <kvengine/transaction.h>

namespace kvengine {

Transaction::Transaction(uint64_t txn_id)
    : txn_id_(txn_id),
      state_(TransactionState::RUNNING),
      start_lsn_(0) {
}

Transaction::~Transaction() {
}

void Transaction::add_write_key(const std::string& key) {
    write_keys_.push_back(key);
}

} // namespace kvengine
