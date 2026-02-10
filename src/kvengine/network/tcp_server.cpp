#include "kvengine/network/tcp_server.h"
#include <iostream>
#include <chrono>

namespace kvengine {
namespace network {

TcpServer::TcpServer(uint16_t port, const std::string& ip)
    : port_(port), ip_(ip), running_(false) {
}

TcpServer::~TcpServer() {
    stop();
}

bool TcpServer::start() {
    if (running_) return false;

    if (!listen_socket_.create()) {
        std::cerr << "TcpServer: Failed to create socket" << std::endl;
        return false;
    }

    listen_socket_.set_reuse_addr(true);
    
    // 綁定地址
    if (!listen_socket_.bind(port_, ip_)) {
        std::cerr << "TcpServer: Failed to bind to " << ip_ << ":" << port_ << std::endl;
        return false;
    }

    // 開始監聽
    if (!listen_socket_.listen()) {
        std::cerr << "TcpServer: Failed to listen" << std::endl;
        return false;
    }

    running_ = true;
    
    // 啟動接受連接的線程（或者是直接在 run() 中執行，取決於設計）
    // 這裡我們選擇在 calling thread 調用 run() 時執行 accept loop
    // 或者我們可以啟動一個後台線程。
    // 根據頭文件 run() 是阻塞的，所以設計意圖是在主線程（或調用線程）運行 accept loop。
    
    std::cout << "TcpServer started on " << ip_ << ":" << port_ << std::endl;
    return true;
}

void TcpServer::stop() {
    if (!running_) return;
    
    running_ = false;
    listen_socket_.close();
    // TODO: 管理活躍連接並優雅關閉
}

void TcpServer::set_connection_handler(ConnectionHandler handler) {
    connection_handler_ = handler;
}

void TcpServer::run() {
    if (!running_) {
        std::cerr << "TcpServer: Not running" << std::endl;
        return;
    }

    accept_loop();
}

void TcpServer::accept_loop() {
    while (running_) {
        SocketAddress client_addr;
        Socket client_socket = listen_socket_.accept(client_addr);

        if (!running_) break;

        if (client_socket.is_valid()) {
            if (connection_handler_) {
                // Thread-per-client model
                std::thread([this, s = std::move(client_socket), addr = client_addr]() mutable {
                    connection_handler_(std::move(s), addr);
                }).detach();
            } else {
                std::cerr << "TcpServer: No connection handler set, closing connection." << std::endl;
                client_socket.close();
            }
        } else {
            // Accept failed or timeout (if using select/poll)
            // For now, simple blocking accept just returns invalid on error or close
            if (running_) {
                 // std::cerr << "TcpServer: Accept failed" << std::endl;
                 // 避免在關閉時刷屏報錯
                 std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }
    }
}

} // namespace network
} // namespace kvengine
