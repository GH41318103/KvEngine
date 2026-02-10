/**
 * @file tcp_server.h
 * @brief TCP 服務器封裝
 * @details 管理服務器生命週期，監聽端口並分發連接
 */

#ifndef KVENGINE_NETWORK_TCP_SERVER_H
#define KVENGINE_NETWORK_TCP_SERVER_H

#include "socket.h"
#include <string>
#include <functional>
#include <atomic>
#include <thread>
#include <vector>
#include <mutex>

namespace kvengine {
namespace network {

class TcpServer {
public:
    /**
     * @brief 構造函數
     * @param port 監聽端口
     * @param ip 綁定 IP (默認 0.0.0.0)
     */
    TcpServer(uint16_t port, const std::string& ip = "0.0.0.0");
    
    ~TcpServer();

    /**
     * @brief 啟動服務器
     * @return 成功返回 true
     */
    bool start();

    /**
     * @brief 停止服務器
     */
    void stop();

    /**
     * @brief 設置連接處理回調
     * @param handler 連接處理函數 (Socket, SocketAddress)
     */
    using ConnectionHandler = std::function<void(Socket client_socket, SocketAddress client_addr)>;
    void set_connection_handler(ConnectionHandler handler);

    /**
     * @brief 運行服務器循環（阻塞）
     */
    void run();

private:
    uint16_t port_;
    std::string ip_;
    Socket listen_socket_;
    std::atomic<bool> running_;
    ConnectionHandler connection_handler_;
    
    void accept_loop();
};

} // namespace network
} // namespace kvengine

#endif // KVENGINE_NETWORK_TCP_SERVER_H
