/**
 * @file resp_parser.h
 * @brief RESP (Redis Serialization Protocol) 解析器
 * @details 負責解析 Redis 客戶端發送的請求
 */

#ifndef KVENGINE_NETWORK_RESP_PARSER_H
#define KVENGINE_NETWORK_RESP_PARSER_H

#include <string>
#include <vector>
#include <cstdint>

namespace kvengine {
namespace network {

/**
 * @class RespParser
 * @brief RESP 協議解析器
 */
class RespParser {
public:
    enum class Status {
        OK,
        INCOMPLETE,
        PARSE_ERROR
    };

    RespParser() = default;

    /**
     * @brief 解析請求
     * @param buffer 輸入緩衝區
     * @param len 緩衝區長度
     * @param out_command 解析出的命令參數列表 (輸出)
     * @param consumed 已消費的字節數 (輸出)
     * @return 解析狀態 (OK: 成功, INCOMPLETE: 數據不足, ERROR: 格式錯誤)
     */
    Status parse(const char* buffer, size_t len, std::vector<std::string>& out_command, size_t& consumed);

private:
    Status parse_array(const char* buffer, size_t len, std::vector<std::string>& out_command, size_t& consumed);
    Status parse_bulk_string(const char* buffer, size_t len, std::string& out_str, size_t& consumed);
    Status parse_int(const char* buffer, size_t len, int64_t& out_val, size_t& consumed);
    
    // 輔助函數：查找 CRLF
    const char* find_crlf(const char* buffer, size_t len);
};

} // namespace network
} // namespace kvengine

#endif // KVENGINE_NETWORK_RESP_PARSER_H
