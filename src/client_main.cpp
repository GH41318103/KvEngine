
#include <kvengine/network/socket.h>
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <stdexcept>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

using namespace kvengine::network;

// ANSI Colors
#define COL_GREEN "\033[32m"
#define COL_RED "\033[31m"
#define COL_RESET "\033[0m"
#define COL_YELLOW "\033[33m"
#define COL_CYAN "\033[36m"
#define COL_GRAY "\033[90m"

void enable_virtual_terminal() {
#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) return;
    
    DWORD dwMode = 0;
    if (!GetConsoleMode(hOut, &dwMode)) return;

    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
    
    // Set Output CP to UTF-8
    SetConsoleOutputCP(65001);
#endif
}

// Helper to buffer reads and handle lines
class BufferedSocketReader {
    Socket& sock_;
    std::vector<char> buffer_;
    size_t pos_ = 0;
    size_t size_ = 0;
public:
    BufferedSocketReader(Socket& sock) : sock_(sock), buffer_(4096) {}
    
    char read_char() {
        if (pos_ >= size_) {
            pos_ = 0;
            int n = sock_.recv(buffer_.data(), static_cast<int>(buffer_.size()));
            if (n <= 0) throw std::runtime_error("Connection closed by server");
            size_ = n;
        }
        return buffer_[pos_++];
    }

    // Read until \r\n, returns string WITHOUT \r\n
    std::string read_line() {
        std::string line;
        while (true) {
            char c = read_char();
            if (c == '\n') break; 
            if (c != '\r') line += c;
        }
        return line;
    }

    // Read N bytes
    std::string read_bytes(size_t n) {
        std::string res;
        res.reserve(n);
        for (size_t i = 0; i < n; ++i) {
            res += read_char();
        }
        return res;
    }
};

class KvClient {
    std::string host = "127.0.0.1";
    uint16_t port = 6379;
    Socket sock;
    BufferedSocketReader* reader = nullptr;

public:
    KvClient(std::string h, uint16_t p) : host(h), port(p) {}
    
    ~KvClient() {
        delete reader; 
        sock.close();
    }

    bool connect() {
        if (!sock.create()) return false;
        if (!sock.connect(host, port)) return false;
        reader = new BufferedSocketReader(sock);
        return true;
    }

    void run_repl() {
        std::string line;
        while (true) {
            std::cout << host << ":" << port << "> " << std::flush;
            
            if (!std::getline(std::cin, line)) break; // EOF
            if (line.empty()) continue;

            // Trim leading/trailing spaces
            size_t first = line.find_first_not_of(" \t");
            if (first == std::string::npos) continue;
            size_t last = line.find_last_not_of(" \t");
            line = line.substr(first, (last - first + 1));

            if (line == "quit" || line == "exit") break;
            if (line == "help") {
                print_help();
                continue;
            }

            try {
                send_command(line);
                print_response(0);
            } catch (const std::exception& e) {
                std::cerr << COL_RED << "Error: " << e.what() << COL_RESET << std::endl;
                break; // Exit on connection error
            }
        }
    }

private:
    void print_help() {
        std::cout << COL_YELLOW << "KvClient Help:" << COL_RESET << std::endl;
        std::cout << "  SET key value   - Set a key" << std::endl;
        std::cout << "  GET key         - Get a key" << std::endl;
        std::cout << "  DEL key         - Delete a key" << std::endl;
        std::cout << "  KEYS pattern    - Find keys (e.g. KEYS *)" << std::endl;
        std::cout << "  PING            - Test connection" << std::endl;
        std::cout << "  quit / exit     - Exit client" << std::endl;
    }

    void send_command(const std::string& line) {
        std::vector<std::string> args;
        std::stringstream ss(line);
        std::string arg;
        while (ss >> arg) args.push_back(arg);

        std::string cmd = "*" + std::to_string(args.size()) + "\r\n";
        for (const auto& part : args) {
            cmd += "$" + std::to_string(part.length()) + "\r\n" + part + "\r\n";
        }
        
        if (sock.send(cmd.c_str(), cmd.size()) <= 0) {
            throw std::runtime_error("Send failed");
        }
    }

    // Recursive generic RESP parser & printer
    void print_response(int depth) {
        char type = reader->read_char();
        std::string line = reader->read_line(); // Read rest of line (e.g. "OK" or length)

        if (type == '+') { // Simple String
            if (line == "OK" || line == "PONG") std::cout << COL_GREEN << line << COL_RESET << std::endl;
            else std::cout << line << std::endl;
        } 
        else if (type == '-') { // Error
            std::cout << COL_RED << "(error) " << line << COL_RESET << std::endl;
        } 
        else if (type == ':') { // Integer
            std::cout << COL_CYAN << "(integer) " << line << COL_RESET << std::endl;
        } 
        else if (type == '$') { // Bulk String
            long long len = std::stoll(line);
            if (len == -1) {
                std::cout << COL_GRAY << "(nil)" << COL_RESET << std::endl;
            } else {
                std::string val = reader->read_bytes(len);
                reader->read_bytes(2); // Consume CRLF
                
                if (depth > 0) std::cout << "\"" << val << "\"" << std::endl;
                else std::cout << val << std::endl;
            }
        } 
        else if (type == '*') { // Array
            long long count = std::stoll(line);
            if (count == -1) {
                std::cout << COL_GRAY << "(nil)" << COL_RESET << std::endl;
            } else {
                for (long long i = 0; i < count; ++i) {
                    // Indent logic for nested output
                    std::cout << std::string(depth * 2, ' ') << (i + 1) << ") ";
                    print_response(depth + 1);
                }
            }
        } else {
            throw std::runtime_error("Unknown RESP type: " + std::string(1, type));
        }
    }
};

int main(int argc, char* argv[]) {
    enable_virtual_terminal();
    if (!Socket::initialize_network()) return 1;

    std::string host = "127.0.0.1";
    uint16_t port = 6379;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" && i + 1 < argc) host = argv[++i];
        else if (arg == "-p" && i + 1 < argc) port = static_cast<uint16_t>(std::stoi(argv[++i]));
    }

    KvClient client(host, port);
    
    std::cout << "Connecting to " << host << ":" << port << "..." << std::endl;
    if (!client.connect()) {
        std::cerr << COL_RED << "Failed to connect to server. Is it running?" << COL_RESET << std::endl;
        return 1;
    }
    
    std::cout << COL_GREEN << "Connected!" << COL_RESET << " Type commands (e.g., SET k v). Ctrl+C to exit." << std::endl;
    
    client.run_repl();

    Socket::cleanup_network();
    return 0;
}
