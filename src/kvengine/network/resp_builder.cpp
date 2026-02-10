#include "kvengine/network/resp_builder.h"

namespace kvengine {
namespace network {

std::string RespBuilder::simple_string(const std::string& str) {
    return "+" + str + "\r\n";
}

std::string RespBuilder::error(const std::string& msg) {
    return "-" + msg + "\r\n";
}

std::string RespBuilder::integer(int64_t val) {
    return ":" + std::to_string(val) + "\r\n";
}

std::string RespBuilder::bulk_string(const std::string& str) {
    return "$" + std::to_string(str.length()) + "\r\n" + str + "\r\n";
}

std::string RespBuilder::null_bulk_string() {
    return "$-1\r\n";
}

std::string RespBuilder::array(const std::vector<std::string>& elements) {
    std::string res = "*" + std::to_string(elements.size()) + "\r\n";
    for (const auto& elem : elements) {
        res += bulk_string(elem);
    }
    return res;
}

std::string RespBuilder::null_array() {
    return "*-1\r\n";
}

} // namespace network
} // namespace kvengine
