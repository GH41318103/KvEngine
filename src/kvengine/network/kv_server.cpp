#include "kvengine/network/kv_server.h"
#include "kvengine/network/resp_parser.h"
#include <iostream>

namespace kvengine {
namespace network {

const size_t BUFFER_SIZE = 8192;

KvServer::KvServer(const std::string& data_dir, uint16_t port, const std::string& host)
    : data_dir_(data_dir), port_(port), host_(host) {
    
    engine_ = std::make_unique<KvEngine>(data_dir_);
    server_ = std::make_unique<TcpServer>(port_, host_);
    // CommandDispatcher requires valid engine pointer, initialized later in start() or here?
    // Engine is not open yet. But pointer is valid.
    dispatcher_ = std::make_unique<CommandDispatcher>(engine_.get());
}

KvServer::~KvServer() {
    stop();
}

bool KvServer::start() {
    if (!engine_->open()) {
        std::cerr << "KvServer: Failed to open engine at " << data_dir_ << std::endl;
        return false;
    }

    server_->set_connection_handler([this](Socket client, SocketAddress addr) {
        std::cout << "Accepted connection from " << addr.to_string() << std::endl;
        
        char buffer[BUFFER_SIZE];
        std::string parsing_buffer; 
        RespParser parser;
        
        while (true) {
            int bytes_read = client.recv(buffer, BUFFER_SIZE);
            if (bytes_read <= 0) break;
            
            parsing_buffer.append(buffer, bytes_read);
            
            size_t total_consumed = 0;
            while (total_consumed < parsing_buffer.size()) {
                std::vector<std::string> command;
                size_t consumed = 0;
                
                auto status = parser.parse(parsing_buffer.c_str() + total_consumed, 
                                          parsing_buffer.size() - total_consumed, 
                                          command, consumed);
                
                if (status == RespParser::Status::OK) {
                    std::string response = dispatcher_->dispatch(command);
                    client.send(response.c_str(), response.size());
                    total_consumed += consumed;
                } else if (status == RespParser::Status::INCOMPLETE) {
                    break; 
                } else {
                    std::cerr << "Protocol error from " << addr.to_string() << std::endl;
                    client.close();
                    return;
                }
            }
            
            if (total_consumed > 0) {
                parsing_buffer.erase(0, total_consumed);
            }
        }
        
        std::cout << "Connection closed: " << addr.to_string() << std::endl;
        client.close(); 
    });

    if (!server_->start()) {
         std::cerr << "KvServer: Failed to start TCP server on " << port_ << std::endl;
         return false;
    }

    return true;
}

void KvServer::stop() {
    server_->stop();
    engine_->close();
}

void KvServer::run() {
    server_->run();
}

} // namespace network
} // namespace kvengine
