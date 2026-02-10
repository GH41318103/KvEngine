#include <kvengine/network/kv_server.h>
#include <kvengine/network/socket.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <cassert>
#include <vector>

using namespace kvengine::network;

// Helper to send command and receive response
std::string send_command(Socket& client, const std::string& cmd) {
    client.send(cmd.c_str(), cmd.size());
    char buffer[1024];
    int bytes = client.recv(buffer, sizeof(buffer));
    if (bytes > 0) return std::string(buffer, bytes);
    return "";
}

void run_client_tests(uint16_t port) {
    std::this_thread::sleep_for(std::chrono::milliseconds(200)); // Wait for server

    Socket client;
    if (!client.create()) {
        std::cerr << "Client: Failed to create socket" << std::endl;
        std::abort();
    }
    if (!client.connect("127.0.0.1", port)) {
        std::cerr << "Client: Failed to connect" << std::endl;
        std::abort();
    }
    
    std::cout << "Client connected." << std::endl;

    // Test PING
    std::string resp = send_command(client, "*1\r\n$4\r\nPING\r\n");
    std::cout << "PING -> " << resp;
    if (resp != "+PONG\r\n") std::abort();
    
    // Test SET
    resp = send_command(client, "*3\r\n$3\r\nSET\r\n$3\r\nfoo\r\n$3\r\nbar\r\n");
    std::cout << "SET foo bar -> " << resp;
    if (resp != "+OK\r\n") std::abort();
    
    // Test GET
    resp = send_command(client, "*2\r\n$3\r\nGET\r\n$3\r\nfoo\r\n");
    std::cout << "GET foo -> " << resp;
    if (resp != "$3\r\nbar\r\n") std::abort();
    
    // Test DEL
    resp = send_command(client, "*2\r\n$3\r\nDEL\r\n$3\r\nfoo\r\n");
    std::cout << "DEL foo -> " << resp;
    if (resp != ":1\r\n") std::abort();
    
    // Test GET (Not Found)
    resp = send_command(client, "*2\r\n$3\r\nGET\r\n$3\r\nfoo\r\n");
    std::cout << "GET foo (after DEL) -> " << resp;
    if (resp != "$-1\r\n") std::abort();

    // Test KEYS
    send_command(client, "*3\r\n$3\r\nSET\r\n$2\r\nk1\r\n$2\r\nv1\r\n");
    send_command(client, "*3\r\n$3\r\nSET\r\n$2\r\nk2\r\n$2\r\nv2\r\n");
    resp = send_command(client, "*2\r\n$4\r\nKEYS\r\n$1\r\n*\r\n");
    std::cout << "KEYS * -> " << resp;
    // Order is not guaranteed, but we expect array of length 2
    if (resp.rfind("*2\r\n") != 0) std::abort();
    if (resp.find("k1") == std::string::npos) std::abort();
    if (resp.find("k2") == std::string::npos) std::abort();

    client.close();
}

int main() {
    if (!Socket::initialize_network()) return 1;

    std::string test_dir = "./test_kv_server_data";
    uint16_t port = 9997;
    
    KvServer server(test_dir, port);
    
    std::thread server_thread([&server]() {
        if (server.start()) {
            server.run();
        }
    });

    std::thread client_thread(run_client_tests, port);
    client_thread.join();
    
    server.stop();
    server_thread.join();
    
    Socket::cleanup_network();
    std::cout << "KvServer integration tests passed!" << std::endl;
    return 0;
}
