/**
 * @file socket.h
 * @brief 網絡套接字封裝 (Socket Wrapper)
 * @details 提供跨平台（Windows/Linux）的 Socket 抽象類
 */

#ifndef KVENGINE_NETWORK_SOCKET_H
#define KVENGINE_NETWORK_SOCKET_H

#include <string>
#include <memory>
#include <cstdint>

// 平台相關的定義
#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    using socket_t = SOCKET;
    #define INVALID_SOCKET_VAL INVALID_SOCKET
#else
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <netdb.h>
    #include <fcntl.h>
    using socket_t = int;
    #define INVALID_SOCKET_VAL -1
#endif

namespace kvengine {
namespace network {

/**
 * @class SocketAddress
 * @brief 網絡地址封裝
 */
class SocketAddress {
public:
    SocketAddress() = default;
    SocketAddress(const std::string& ip, uint16_t port);
    explicit SocketAddress(const struct sockaddr_in& addr);

    std::string to_string() const;
    std::string get_ip() const;
    uint16_t get_port() const;
    struct sockaddr_in get_addr() const { return addr_; }

private:
    struct sockaddr_in addr_{};
};

/**
 * @class Socket
 * @brief RAII 風格的 Socket 封裝類
 */
class Socket {
public:
    Socket();
    explicit Socket(socket_t handle);
    ~Socket();

    // 禁用拷貝（Socket 是獨占資源）
    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;

    // 允許移動
    Socket(Socket&& other) noexcept;
    Socket& operator=(Socket&& other) noexcept;

    /**
     * @brief 創建 TCP Socket
     * @return 成功返回 true
     */
    bool create();

    /**
     * @brief 綁定地址
     * @param port 端口號
     * @param ip IP 地址（默認為 "0.0.0.0"）
     * @return 成功返回 true
     */
    bool bind(uint16_t port, const std::string& ip = "0.0.0.0");

    /**
     * @brief 開始監聽
     * @param backlog 等待隊列長度
     * @return 成功返回 true
     */
    bool listen(int backlog = SOMAXCONN);

    /**
     * @brief 接受連接
     * @param client_addr 輸出參數，客戶端地址
     * @return 客戶端 Socket 對象（如果失敗，返回的對象無效）
     */
    Socket accept(SocketAddress& client_addr);

    /**
     * @brief 連接到服務器
     * @param ip 服務器 IP
     * @param port 服務器端口
     * @return 成功返回 true
     */
    bool connect(const std::string& ip, uint16_t port);

    /**
     * @brief 發送數據
     * @param data 數據指針
     * @param len 數據長度
     * @return 發送的字節數，-1 表示錯誤
     */
    int send(const char* data, size_t len);

    /**
     * @brief 接收數據
     * @param buffer 緩衝區
     * @param len 緩衝區長度
     * @return 接收的字節數，0 表示連接關閉，-1 表示錯誤
     */
    int recv(char* buffer, size_t len);

    /**
     * @brief 關閉 Socket
     */
    void close();

    /**
     * @brief 檢查 Socket 是否有效
     */
    bool is_valid() const;

    /**
     * @brief 設置非阻塞模式
     */
    bool set_non_blocking(bool on = true);

    /**
     * @brief 設置地址重用
     */
    bool set_reuse_addr(bool on = true);

    /**
     * @brief 獲取原生句柄
     */
    socket_t get_handle() const { return handle_; }

    // 全局初始化/清理（Windows 需要）
    static bool initialize_network();
    static void cleanup_network();

private:
    socket_t handle_ = INVALID_SOCKET_VAL;
};

} // namespace network
} // namespace kvengine

#endif // KVENGINE_NETWORK_SOCKET_H
