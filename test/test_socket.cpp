#include <kvengine/network/socket.h>
#include <iostream>
#include <thread>
#include <vector>
#include <string>
#include <cassert>
#include <chrono>

using namespace kvengine::network;

void run_server(uint16_t port) {
    Socket server;
    if (!server.create()) {
        std::cerr << "Server: Failed to create socket" << std::endl;
        std::abort();
    }
    
    // Allow address reuse to avoid "Address already in use" errors during testing
    server.set_reuse_addr(true);

    if (!server.bind(port)) {
        std::cerr << "Server: Failed to bind to port " << port << std::endl;
        std::abort();
    }

    if (!server.listen()) {
        std::cerr << "Server: Failed to listen" << std::endl;
        std::abort();
    }

    std::cout << "Server: Listening on port " << port << std::endl;

    SocketAddress client_addr;
    Socket client = server.accept(client_addr);
    if (!client.is_valid()) {
        std::cerr << "Server: Failed to accept connection" << std::endl;
        return;
    }

    std::cout << "Server: Accepted connection from " << client_addr.to_string() << std::endl;

    char buffer[1024];
    int bytes_read = client.recv(buffer, sizeof(buffer));
    if (bytes_read > 0) {
        std::string msg(buffer, bytes_read);
        std::cout << "Server: Received: " << msg << std::endl;
        
        std::string response = "PONG";
        client.send(response.c_str(), response.size());
    }
}

void run_client(uint16_t port) {
    // Wait for server to start
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    Socket client;
    if (!client.create()) {
        std::cerr << "Client: Failed to create socket" << std::endl;
        std::abort();
    }

    if (!client.connect("127.0.0.1", port)) {
        std::cerr << "Client: Failed to connect" << std::endl;
        std::abort();
    }

    std::string msg = "PING";
    client.send(msg.c_str(), msg.size());

    char buffer[1024];
    int bytes_read = client.recv(buffer, sizeof(buffer));
    if (bytes_read > 0) {
        std::string response(buffer, bytes_read);
        std::cout << "Client: Received: " << response << std::endl;
        
        if (response != "PONG") {
            std::cerr << "Client: Unexpected response: " << response << std::endl;
            std::abort();
        }
    } else {
        std::cerr << "Client: Failed to receive response" << std::endl;
        std::abort();
    }
}

int main() {
    // Initialize network (WSA on Windows)
    if (!Socket::initialize_network()) {
        std::cerr << "Failed to initialize network" << std::endl;
        return 1;
    }

    uint16_t port = 9999;
    
    std::thread server_thread(run_server, port);
    std::thread client_thread(run_client, port);

    server_thread.join();
    client_thread.join();

    Socket::cleanup_network();
    
    std::cout << "Socket test passed!" << std::endl;
    return 0;
}
