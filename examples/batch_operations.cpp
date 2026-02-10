#include <kvengine/kv_engine.h>
#include <iostream>
#include <map>
#include <chrono>

using namespace kvengine;

int main() {
    try {
        std::cout << "=== KvEngine Batch Operations Example ===" << std::endl << std::endl;
        
        KvEngine engine("./data");
        
        if (!engine.open()) {
            std::cerr << "Failed to open engine" << std::endl;
            return 1;
        }
        
        // Prepare batch data
        std::map<std::string, std::string> batch;
        const int NUM_ITEMS = 1000;
        
        std::cout << "Preparing " << NUM_ITEMS << " items for batch insert..." << std::endl;
        for (int i = 0; i < NUM_ITEMS; ++i) {
            std::string key = "config:item:" + std::to_string(i);
            std::string value = "value_" + std::to_string(i);
            batch[key] = value;
        }
        
        // Method 1: Individual puts
        std::cout << std::endl << "Method 1: Individual puts..." << std::endl;
        auto start1 = std::chrono::high_resolution_clock::now();
        
        for (const auto& pair : batch) {
            engine.put(pair.first, pair.second);
        }
        
        auto end1 = std::chrono::high_resolution_clock::now();
        auto duration1 = std::chrono::duration_cast<std::chrono::milliseconds>(end1 - start1);
        std::cout << "  Time: " << duration1.count() << " ms" << std::endl;
        
        // Clear data
        for (const auto& pair : batch) {
            engine.remove(pair.first);
        }
        
        // Method 2: Batch put
        std::cout << std::endl << "Method 2: Batch put..." << std::endl;
        auto start2 = std::chrono::high_resolution_clock::now();
        
        engine.batch_put(batch);
        
        auto end2 = std::chrono::high_resolution_clock::now();
        auto duration2 = std::chrono::duration_cast<std::chrono::milliseconds>(end2 - start2);
        std::cout << "  Time: " << duration2.count() << " ms" << std::endl;
        
        // Performance comparison
        std::cout << std::endl << "Performance comparison:" << std::endl;
        std::cout << "  Individual puts: " << duration1.count() << " ms" << std::endl;
        std::cout << "  Batch put: " << duration2.count() << " ms" << std::endl;
        
        if (duration2.count() > 0) {
            double speedup = static_cast<double>(duration1.count()) / duration2.count();
            std::cout << "  Speedup: " << speedup << "x" << std::endl;
        }
        
        // Verify data
        std::cout << std::endl << "Verifying data..." << std::endl;
        int count = 0;
        auto iterator = engine.scan("config:item:");
        while (iterator->valid()) {
            count++;
            iterator->next();
        }
        std::cout << "  Found " << count << " items with prefix 'config:item:'" << std::endl;
        
        // Statistics
        Statistics stats = engine.get_statistics();
        std::cout << std::endl << "Statistics:" << std::endl;
        std::cout << "  Total keys: " << stats.total_keys << std::endl;
        std::cout << "  Memory used: " << stats.memory_used << " bytes" << std::endl;
        
        engine.close();
        
        std::cout << std::endl << "=== Example completed successfully! ===" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
