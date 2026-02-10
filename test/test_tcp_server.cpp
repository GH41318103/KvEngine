#include <kvengine/network/tcp_server.h>
#include <iostream>
#include <thread>
#include <string>
#include <chrono>

using namespace kvengine::network;

void run_test_client(uint16_t port) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Wait for server

    Socket client;
    if (!client.create()) {
        std::cerr << "Client: Failed to create socket" << std::endl;
        std::abort();
    }

    if (!client.connect("127.0.0.1", port)) {
        std::cerr << "Client: Failed to connect" << std::endl;
        std::abort();
    }

    std::string msg = "HELLO_SERVER";
    client.send(msg.c_str(), msg.size());

    char buffer[1024];
    int bytes_read = client.recv(buffer, sizeof(buffer));
    if (bytes_read > 0) {
        std::string response(buffer, bytes_read);
        std::cout << "Client: Received: " << response << std::endl;
        if (response != "HELLO_CLIENT") {
             std::cerr << "Client: Unexpected response: " << response << std::endl;
             std::abort();
        }
    } else {
        std::cerr << "Client: Failed to receive response" << std::endl;
        std::abort();
    }
}

int main() {
    if (!Socket::initialize_network()) {
        std::cerr << "Failed to initialize network" << std::endl;
        return 1;
    }

    uint16_t port = 9998;
    TcpServer server(port);

    server.set_connection_handler([](Socket client, SocketAddress addr) {
        std::cout << "Server Handler: Connection from " << addr.to_string() << std::endl;
        
        char buffer[1024];
        int bytes = client.recv(buffer, sizeof(buffer));
        if (bytes > 0) {
            std::string msg(buffer, bytes);
            std::cout << "Server Handler: Received: " << msg << std::endl;
            
            std::string response = "HELLO_CLIENT";
            client.send(response.c_str(), response.size());
        }
        client.close();
    });

    std::thread server_thread([&server]() {
        if (server.start()) {
            server.run();
        }
    });

    std::thread client_thread(run_test_client, port);
    client_thread.join();

    server.stop();
    server_thread.join();

    Socket::cleanup_network();
    std::cout << "TcpServer test passed!" << std::endl;
    return 0;
}
