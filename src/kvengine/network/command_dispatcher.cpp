#include "kvengine/network/command_dispatcher.h"
#include "kvengine/network/resp_builder.h"
#include <algorithm>
#include <iostream>

namespace kvengine {
namespace network {

CommandDispatcher::CommandDispatcher(KvEngine* engine) : engine_(engine) {}

std::string CommandDispatcher::dispatch(const std::vector<std::string>& command) {
    if (command.empty()) {
        return RespBuilder::error("ERR empty command");
    }

    std::string cmd_name = command[0];
    std::transform(cmd_name.begin(), cmd_name.end(), cmd_name.begin(), ::toupper);

    if (cmd_name == "PING") {
        return handle_ping(command);
    } else if (cmd_name == "SET") {
        return handle_set(command);
    } else if (cmd_name == "GET") {
        return handle_get(command);
    } else if (cmd_name == "DEL") {
        return handle_del(command);
    } else if (cmd_name == "KEYS") {
        return handle_keys(command);
    } else {
        return handle_unknown(cmd_name);
    }
}

std::string CommandDispatcher::handle_ping(const std::vector<std::string>& command) {
    if (command.size() > 2) {
        return RespBuilder::error("ERR wrong number of arguments for 'ping' command");
    }
    if (command.size() == 2) {
        return RespBuilder::bulk_string(command[1]);
    }
    return RespBuilder::simple_string("PONG");
}

std::string CommandDispatcher::handle_set(const std::vector<std::string>& command) {
    if (command.size() != 3) {
        return RespBuilder::error("ERR wrong number of arguments for 'set' command");
    }
    
    const std::string& key = command[1];
    const std::string& value = command[2];
    
    if (engine_->put(key, value)) {
        return RespBuilder::simple_string("OK");
    } else {
        return RespBuilder::error("ERR failed to set key");
    }
}

std::string CommandDispatcher::handle_get(const std::vector<std::string>& command) {
    if (command.size() != 2) {
        return RespBuilder::error("ERR wrong number of arguments for 'get' command");
    }
    
    const std::string& key = command[1];
    std::string value;
    Status s = engine_->get(key, value);
    
    if (s.ok()) {
        return RespBuilder::bulk_string(value);
    } else if (s.is_not_found()) {
        return RespBuilder::null_bulk_string();
    } else {
        return RespBuilder::error("ERR " + s.to_string());
    }
}

std::string CommandDispatcher::handle_del(const std::vector<std::string>& command) {
    if (command.size() < 2) {
        return RespBuilder::error("ERR wrong number of arguments for 'del' command");
    }
    
    int count = 0;
    for (size_t i = 1; i < command.size(); ++i) {
        if (engine_->remove(command[i])) {
            count++;
        }
    }
    
    return RespBuilder::integer(count);
}

std::string CommandDispatcher::handle_keys(const std::vector<std::string>& command) {
    if (command.size() != 2) {
        return RespBuilder::error("ERR wrong number of arguments for 'keys' command");
    }

    // 當前 scan 實現僅支持前綴掃描，Redis KEYS 支持 glob 模式。
    // 為了簡單起見，我們如果參數是 "*" 則掃描所有，否則把參數當作前綴。
    // 這不是嚴格的 Redis 行為，但對於原型來說足夠了。

    std::string pattern = command[1];
    std::string prefix = pattern;
    if (pattern == "*") {
        prefix = "";
    } else {
        // 如果末尾有 *，去掉它當作前綴
        if (!prefix.empty() && prefix.back() == '*') {
            prefix.pop_back();
        }
    }

    auto iter = engine_->scan(prefix);
    std::vector<std::string> keys;
    while (iter->valid()) {
        keys.push_back(iter->key());
        iter->next();
    }

    return RespBuilder::array(keys);
}

std::string CommandDispatcher::handle_unknown(const std::string& cmd_name) {
    return RespBuilder::error("ERR unknown command '" + cmd_name + "'");
}

} // namespace network
} // namespace kvengine
