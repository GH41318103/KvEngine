#include <kvengine/network/kv_server.h>
#include <kvengine/network/socket.h>
#include <iostream>
#include <string>

using namespace kvengine::network;

int main(int argc, char* argv[]) {
    std::string data_dir = "./data";
    uint16_t port = 6379;
    std::string host = "0.0.0.0";

    if (argc > 1) port = static_cast<uint16_t>(std::stoi(argv[1]));
    if (argc > 2) data_dir = argv[2];

    if (!Socket::initialize_network()) {
        std::cerr << "Failed to initialize network" << std::endl;
        return 1;
    }

    KvServer server(data_dir, port, host);
    
    if (server.start()) {
        std::cout << "KvServer is running on " << host << ":" << port << "..." << std::endl;
        server.run();
    } else {
        std::cerr << "Failed to start server." << std::endl;
    }

    Socket::cleanup_network();
    return 0;
}
