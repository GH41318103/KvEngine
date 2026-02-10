#include <kvengine/network/resp_parser.h>
#include <kvengine/network/resp_builder.h>
#include <iostream>
#include <cassert>
#include <string>
#include <vector>

using namespace kvengine::network;

void test_resp_builder() {
    std::cout << "Testing RespBuilder..." << std::endl;
    
    // Simple String
    assert(RespBuilder::simple_string("OK") == "+OK\r\n");
    
    // Error
    assert(RespBuilder::error("Error message") == "-Error message\r\n");
    
    // Integer
    assert(RespBuilder::integer(123) == ":123\r\n");
    assert(RespBuilder::integer(-456) == ":-456\r\n");
    
    // Bulk String
    assert(RespBuilder::bulk_string("foobar") == "$6\r\nfoobar\r\n");
    assert(RespBuilder::bulk_string("") == "$0\r\n\r\n");
    
    // Null Bulk String
    assert(RespBuilder::null_bulk_string() == "$-1\r\n");
    
    // Array
    std::vector<std::string> v = {"foo", "bar"};
    assert(RespBuilder::array(v) == "*2\r\n$3\r\nfoo\r\n$3\r\nbar\r\n");
    
    std::cout << "RespBuilder tests passed!" << std::endl;
}

void test_resp_parser() {
    std::cout << "Testing RespParser..." << std::endl;
    RespParser parser;
    std::vector<std::string> command;
    size_t consumed = 0;
    
    // Test 1: Standard Array
    std::string input = "*3\r\n$3\r\nSET\r\n$3\r\nkey\r\n$5\r\nvalue\r\n";
    auto status = parser.parse(input.c_str(), input.size(), command, consumed);
    
    assert(status == RespParser::Status::OK);
    assert(consumed == input.size());
    assert(command.size() == 3);
    assert(command[0] == "SET");
    assert(command[1] == "key");
    assert(command[2] == "value");
    
    // Test 2: Incomplete data
    std::string partial = "*3\r\n$3\r\nSET\r\n";
    status = parser.parse(partial.c_str(), partial.size(), command, consumed);
    assert(status == RespParser::Status::INCOMPLETE);
    
    // Test 3: Multiple commands in buffer
    std::string two_cmds = "*1\r\n$4\r\nPING\r\n*1\r\n$4\r\nPING\r\n";
    status = parser.parse(two_cmds.c_str(), two_cmds.size(), command, consumed);
    assert(status == RespParser::Status::OK);
    assert(consumed == 14); // length of first command
    assert(command.size() == 1);
    assert(command[0] == "PING");
    
    // Test 4: Invalid format
    std::string invalid = "NOT_RESP\r\n";
    status = parser.parse(invalid.c_str(), invalid.size(), command, consumed);
    assert(status == RespParser::Status::PARSE_ERROR);

    std::cout << "RespParser tests passed!" << std::endl;
}

int main() {
    test_resp_builder();
    test_resp_parser();
    return 0;
}
