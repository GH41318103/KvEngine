#include <kvengine/kv_engine.h>
#include <iostream>
#include <string>

using namespace kvengine;

int main() {
    try {
        std::cout << "=== KvEngine Basic Usage Example ===" << std::endl << std::endl;
        
        // 1. Create engine instance
        std::cout << "1. Creating KvEngine instance..." << std::endl;
        KvEngine engine("./data");
        
        // 2. Open engine
        std::cout << "2. Opening engine..." << std::endl;
        if (!engine.open()) {
            std::cerr << "Failed to open engine" << std::endl;
            return 1;
        }
        std::cout << "   Engine opened successfully!" << std::endl << std::endl;
        
        // 3. Write data
        std::cout << "3. Writing data..." << std::endl;
        engine.put("user:1:name", "John Doe");
        engine.put("user:1:email", "john@example.com");
        engine.put("user:1:age", "30");
        engine.put("user:2:name", "Jane Smith");
        engine.put("user:2:email", "jane@example.com");
        std::cout << "   Data written successfully!" << std::endl << std::endl;
        
        // 4. Read data
        std::cout << "4. Reading data..." << std::endl;
        std::string name = engine.get("user:1:name");
        std::string email = engine.get("user:1:email");
        std::cout << "   User 1: " << name << " (" << email << ")" << std::endl << std::endl;
        
        // 5. Check if key exists
        std::cout << "5. Checking key existence..." << std::endl;
        if (engine.exists("user:1:email")) {
            std::cout << "   Key 'user:1:email' exists!" << std::endl;
        }
        if (!engine.exists("user:999:name")) {
            std::cout << "   Key 'user:999:name' does not exist!" << std::endl;
        }
        std::cout << std::endl;
        
        // 6. Delete data
        std::cout << "6. Deleting data..." << std::endl;
        engine.remove("user:1:age");
        std::cout << "   Key 'user:1:age' removed!" << std::endl;
        if (!engine.exists("user:1:age")) {
            std::cout << "   Verified: key no longer exists" << std::endl;
        }
        std::cout << std::endl;
        
        // 7. Scan with prefix
        std::cout << "7. Scanning keys with prefix 'user:1:'..." << std::endl;
        auto iterator = engine.scan("user:1:");
        while (iterator->valid()) {
            std::cout << "   " << iterator->key() << " = " << iterator->value() << std::endl;
            iterator->next();
        }
        std::cout << std::endl;
        
        // 8. Scan all keys
        std::cout << "8. Scanning all keys..." << std::endl;
        auto all_iterator = engine.scan();
        while (all_iterator->valid()) {
            std::cout << "   " << all_iterator->key() << " = " << all_iterator->value() << std::endl;
            all_iterator->next();
        }
        std::cout << std::endl;
        
        // 9. Get statistics
        std::cout << "9. Getting statistics..." << std::endl;
        Statistics stats = engine.get_statistics();
        std::cout << "   Total keys: " << stats.total_keys << std::endl;
        std::cout << "   Memory used: " << stats.memory_used << " bytes" << std::endl;
        std::cout << "   Total reads: " << stats.total_reads << std::endl;
        std::cout << "   Total writes: " << stats.total_writes << std::endl;
        std::cout << std::endl;
        
        // 10. Flush to disk
        std::cout << "10. Flushing data to disk..." << std::endl;
        if (engine.flush()) {
            std::cout << "    Data flushed successfully!" << std::endl;
        }
        std::cout << std::endl;
        
        // 11. Close engine
        std::cout << "11. Closing engine..." << std::endl;
        engine.close();
        std::cout << "    Engine closed successfully!" << std::endl << std::endl;
        
        std::cout << "=== Example completed successfully! ===" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
