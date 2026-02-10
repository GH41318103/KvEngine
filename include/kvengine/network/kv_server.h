/**
 * @file kv_server.h
 * @brief KvServer 類
 * @details 整合 TcpServer, RespParser, CommandDispatcher 和 KvEngine
 */

#ifndef KVENGINE_NETWORK_KV_SERVER_H
#define KVENGINE_NETWORK_KV_SERVER_H

#include <kvengine/kv_engine.h>
#include <kvengine/network/tcp_server.h>
#include <kvengine/network/command_dispatcher.h>
#include <memory>
#include <string>

namespace kvengine {
namespace network {

class KvServer {
public:
    KvServer(const std::string& data_dir, uint16_t port, const std::string& host = "0.0.0.0");
    ~KvServer();

    bool start();
    void stop();
    void run(); // blocking

private:
    std::string data_dir_;
    uint16_t port_;
    std::string host_;
    
    std::unique_ptr<KvEngine> engine_;
    std::unique_ptr<TcpServer> server_;
    std::unique_ptr<CommandDispatcher> dispatcher_;
};

} // namespace network
} // namespace kvengine

#endif // KVENGINE_NETWORK_KV_SERVER_H
