#include "kvengine/network/resp_parser.h"
#include <cstring>
#include <iostream>

namespace kvengine {
namespace network {

const char* RespParser::find_crlf(const char* buffer, size_t len) {
    for (size_t i = 0; i < len - 1; ++i) {
        if (buffer[i] == '\r' && buffer[i+1] == '\n') {
            return buffer + i;
        }
    }
    return nullptr;
}

RespParser::Status RespParser::parse(const char* buffer, size_t len, 
                                     std::vector<std::string>& out_command, 
                                     size_t& consumed) {
    consumed = 0;
    out_command.clear();
    
    if (len == 0) return Status::INCOMPLETE;
    
    // 客戶端發送的命令通常是 Array 類型
    if (buffer[0] != '*') {
        // 簡單支持 inline command? (e.g. "PING\r\n")
        // 為了簡單起見，我們暫時假設均為標準 RESP Array
        return Status::PARSE_ERROR;
    }
    
    return parse_array(buffer, len, out_command, consumed);
}

RespParser::Status RespParser::parse_int(const char* buffer, size_t len, 
                                        int64_t& out_val, size_t& consumed) {
    const char* crlf = find_crlf(buffer, len);
    if (!crlf) return Status::INCOMPLETE;
    
    size_t line_len = crlf - buffer; // 不包含 CRLF
    std::string num_str(buffer, line_len);
    
    try {
        out_val = std::stoll(num_str);
    } catch (...) {
        return Status::PARSE_ERROR;
    }
    
    consumed = line_len + 2; // +2 for CRLF
    return Status::OK;
}

RespParser::Status RespParser::parse_bulk_string(const char* buffer, size_t len, 
                                                std::string& out_str, size_t& consumed) {
    if (len < 1 || buffer[0] != '$') return Status::PARSE_ERROR;
    
    size_t offset = 1;
    int64_t str_len = 0;
    size_t int_consumed = 0;
    
    // 解析長度
    Status s = parse_int(buffer + offset, len - offset, str_len, int_consumed);
    if (s != Status::OK) return s;
    
    offset += int_consumed;
    
    // 檢查剩餘數據是否足夠 (str_len + CRLF)
    if (str_len == -1) {
        // Null Bulk String
        out_str = ""; // or special value?
        consumed = offset;
        return Status::OK;
    }
    
    if (len - offset < static_cast<size_t>(str_len) + 2) {
        return Status::INCOMPLETE;
    }
    
    // 讀取字符串內容
    out_str.assign(buffer + offset, str_len);
    
    // 驗證結尾 CRLF
    if (buffer[offset + str_len] != '\r' || buffer[offset + str_len + 1] != '\n') {
        return Status::PARSE_ERROR;
    }
    
    consumed = offset + str_len + 2;
    return Status::OK;
}

RespParser::Status RespParser::parse_array(const char* buffer, size_t len, 
                                          std::vector<std::string>& out_command, 
                                          size_t& consumed) {
    if (len < 1 || buffer[0] != '*') return Status::PARSE_ERROR;
    
    size_t offset = 1;
    int64_t array_len = 0;
    size_t int_consumed = 0;
    
    // 解析數組元素個數
    Status s = parse_int(buffer + offset, len - offset, array_len, int_consumed);
    if (s != Status::OK) return s;
    
    offset += int_consumed;
    
    if (array_len < 0) return Status::PARSE_ERROR; // 不能是負數
    
    out_command.reserve(array_len);
    
    for (int i = 0; i < array_len; ++i) {
        if (offset >= len) return Status::INCOMPLETE;
        
        // 期望是 Bulk String ($)
        if (buffer[offset] != '$') {
            return Status::PARSE_ERROR; // 只支持 Bulk Strings elements for commands
        }
        
        std::string elem;
        size_t bulk_consumed = 0;
        s = parse_bulk_string(buffer + offset, len - offset, elem, bulk_consumed);
        if (s != Status::OK) return s;
        
        out_command.push_back(elem);
        offset += bulk_consumed;
    }
    
    consumed = offset;
    return Status::OK;
}

} // namespace network
} // namespace kvengine
