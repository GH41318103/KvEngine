#include "kvengine/network/socket.h"
#include <iostream>
#include <cstring>
#include <stdexcept>

namespace kvengine {
namespace network {

// ===== SocketAddress =====

SocketAddress::SocketAddress(const std::string& ip, uint16_t port) {
    std::memset(&addr_, 0, sizeof(addr_));
    addr_.sin_family = AF_INET;
    addr_.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &addr_.sin_addr);
}

SocketAddress::SocketAddress(const struct sockaddr_in& addr) : addr_(addr) {}

std::string SocketAddress::to_string() const {
    return get_ip() + ":" + std::to_string(get_port());
}

std::string SocketAddress::get_ip() const {
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &addr_.sin_addr, ip, INET_ADDRSTRLEN);
    return std::string(ip);
}

uint16_t SocketAddress::get_port() const {
    return ntohs(addr_.sin_port);
}

// ===== Socket =====

Socket::Socket() : handle_(INVALID_SOCKET_VAL) {}

Socket::Socket(socket_t handle) : handle_(handle) {}

Socket::~Socket() {
    close();
}

Socket::Socket(Socket&& other) noexcept : handle_(other.handle_) {
    other.handle_ = INVALID_SOCKET_VAL;
}

Socket& Socket::operator=(Socket&& other) noexcept {
    if (this != &other) {
        close();
        handle_ = other.handle_;
        other.handle_ = INVALID_SOCKET_VAL;
    }
    return *this;
}

bool Socket::create() {
    close();
    handle_ = ::socket(AF_INET, SOCK_STREAM, 0);
    return is_valid();
}

bool Socket::bind(uint16_t port, const std::string& ip) {
    if (!is_valid()) return false;

    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) <= 0) {
        return false;
    }

    return ::bind(handle_, (struct sockaddr*)&addr, sizeof(addr)) == 0;
}

bool Socket::listen(int backlog) {
    if (!is_valid()) return false;
    return ::listen(handle_, backlog) == 0;
}

Socket Socket::accept(SocketAddress& client_addr) {
    if (!is_valid()) return Socket();

    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    socket_t client_handle = ::accept(handle_, (struct sockaddr*)&addr, &len);

    if (client_handle != INVALID_SOCKET_VAL) {
        client_addr = SocketAddress(addr);
        return Socket(client_handle);
    }
    return Socket();
}

bool Socket::connect(const std::string& ip, uint16_t port) {
    if (!is_valid()) create();

    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) <= 0) {
        return false;
    }

    return ::connect(handle_, (struct sockaddr*)&addr, sizeof(addr)) == 0;
}

int Socket::send(const char* data, size_t len) {
    if (!is_valid()) return -1;
    return ::send(handle_, data, static_cast<int>(len), 0);
}

int Socket::recv(char* buffer, size_t len) {
    if (!is_valid()) return -1;
    return ::recv(handle_, buffer, static_cast<int>(len), 0);
}

void Socket::close() {
    if (handle_ != INVALID_SOCKET_VAL) {
#ifdef _WIN32
        ::closesocket(handle_);
#else
        ::close(handle_);
#endif
        handle_ = INVALID_SOCKET_VAL;
    }
}

bool Socket::is_valid() const {
    return handle_ != INVALID_SOCKET_VAL;
}

bool Socket::set_non_blocking(bool on) {
    if (!is_valid()) return false;
#ifdef _WIN32
    u_long mode = on ? 1 : 0;
    return ioctlsocket(handle_, FIONBIO, &mode) == 0;
#else
    int flags = fcntl(handle_, F_GETFL, 0);
    if (flags == -1) return false;
    if (on) flags |= O_NONBLOCK;
    else flags &= ~O_NONBLOCK;
    return fcntl(handle_, F_SETFL, flags) == 0;
#endif
}

bool Socket::set_reuse_addr(bool on) {
    if (!is_valid()) return false;
    int opt = on ? 1 : 0;
    return ::setsockopt(handle_, SOL_SOCKET, SO_REUSEADDR, 
                        (char*)&opt, sizeof(opt)) == 0;
}

// Global initialization for Windows
bool Socket::initialize_network() {
#ifdef _WIN32
    WSADATA wsa_data;
    return WSAStartup(MAKEWORD(2, 2), &wsa_data) == 0;
#else
    return true;
#endif
}

void Socket::cleanup_network() {
#ifdef _WIN32
    WSACleanup();
#endif
}

} // namespace network
} // namespace kvengine
