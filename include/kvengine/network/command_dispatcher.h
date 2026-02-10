/**
 * @file command_dispatcher.h
 * @brief Redis 命令分發器
 * @details 將解析後的 RESP 命令分發到具體的 KvEngine 操作
 */

#ifndef KVENGINE_NETWORK_COMMAND_DISPATCHER_H
#define KVENGINE_NETWORK_COMMAND_DISPATCHER_H

#include <vector>
#include <string>
#include <kvengine/kv_engine.h>

namespace kvengine {
namespace network {

class CommandDispatcher {
public:
    explicit CommandDispatcher(KvEngine* engine);

    /**
     * @brief 處理命令
     * @param command 命令參數列表 (e.g., {"SET", "key", "value"})
     * @return RESP 格式的響應字符串
     */
    std::string dispatch(const std::vector<std::string>& command);

private:
    KvEngine* engine_;

    std::string handle_ping(const std::vector<std::string>& command);
    std::string handle_set(const std::vector<std::string>& command);
    std::string handle_get(const std::vector<std::string>& command);
    std::string handle_del(const std::vector<std::string>& command);
    std::string handle_keys(const std::vector<std::string>& command);
    std::string handle_unknown(const std::string& cmd_name);
};

} // namespace network
} // namespace kvengine

#endif // KVENGINE_NETWORK_COMMAND_DISPATCHER_H
