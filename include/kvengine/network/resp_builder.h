/**
 * @file resp_builder.h
 * @brief RESP 響應構建器
 * @details 用於構建服務器響應
 */

#ifndef KVENGINE_NETWORK_RESP_BUILDER_H
#define KVENGINE_NETWORK_RESP_BUILDER_H

#include <string>
#include <vector>
#include <cstdint>

namespace kvengine {
namespace network {

class RespBuilder {
public:
    // Simple String: +OK\r\n
    static std::string simple_string(const std::string& str);
    
    // Error: -Error message\r\n
    static std::string error(const std::string& msg);
    
    // Integer: :1000\r\n
    static std::string integer(int64_t val);
    
    // Bulk String: $6\r\nfoobar\r\n
    static std::string bulk_string(const std::string& str);
    
    // Null Bulk String: $-1\r\n
    static std::string null_bulk_string();
    
    // Array: *2\r\n$3\r\nfoo\r\n$3\r\nbar\r\n
    static std::string array(const std::vector<std::string>& elements);
    
    // Null Array: *-1\r\n
    static std::string null_array();
};

} // namespace network
} // namespace kvengine

#endif // KVENGINE_NETWORK_RESP_BUILDER_H
