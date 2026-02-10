#include <kvengine/kv_engine.h>
#include <iostream>
#include <chrono>
#include <random>
#include <iomanip>

using namespace kvengine;

class Benchmark {
public:
    void run_all() {
        std::cout << "=== KvEngine Benchmark Suite ===" << std::endl << std::endl;
        
        benchmark_write();
        benchmark_read();
        benchmark_mixed();
        benchmark_scan();
        
        std::cout << std::endl << "=== Benchmark completed ===" << std::endl;
    }
    
private:
    void benchmark_write() {
        std::cout << "Benchmark: Sequential Write" << std::endl;
        
        KvEngine engine("./bench_data");
        engine.open();
        
        const int NUM_OPS = 10000;
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < NUM_OPS; ++i) {
            std::string key = "key" + std::to_string(i);
            std::string value = "value" + std::to_string(i);
            engine.put(key, value);
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        double ops_per_sec = (NUM_OPS * 1000.0) / duration.count();
        
        std::cout << "  Operations: " << NUM_OPS << std::endl;
        std::cout << "  Time: " << duration.count() << " ms" << std::endl;
        std::cout << "  Throughput: " << std::fixed << std::setprecision(2) 
                  << ops_per_sec << " ops/sec" << std::endl << std::endl;
        
        engine.close();
    }
    
    void benchmark_read() {
        std::cout << "Benchmark: Random Read" << std::endl;
        
        KvEngine engine("./bench_data");
        engine.open();
        
        // Prepare data
        const int NUM_KEYS = 10000;
        for (int i = 0; i < NUM_KEYS; ++i) {
            std::string key = "key" + std::to_string(i);
            std::string value = "value" + std::to_string(i);
            engine.put(key, value);
        }
        
        // Random read
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, NUM_KEYS - 1);
        
        const int NUM_OPS = 50000;
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < NUM_OPS; ++i) {
            int random_key = dis(gen);
            std::string key = "key" + std::to_string(random_key);
            volatile std::string value = engine.get(key);
            (void)value; // Prevent optimization
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        double ops_per_sec = (NUM_OPS * 1000.0) / duration.count();
        
        std::cout << "  Operations: " << NUM_OPS << std::endl;
        std::cout << "  Time: " << duration.count() << " ms" << std::endl;
        std::cout << "  Throughput: " << std::fixed << std::setprecision(2) 
                  << ops_per_sec << " ops/sec" << std::endl << std::endl;
        
        engine.close();
    }
    
    void benchmark_mixed() {
        std::cout << "Benchmark: Mixed Operations (50% read, 50% write)" << std::endl;
        
        KvEngine engine("./bench_data");
        engine.open();
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 1);
        
        const int NUM_OPS = 20000;
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < NUM_OPS; ++i) {
            if (dis(gen) == 0) {
                // Write
                std::string key = "mixed" + std::to_string(i);
                std::string value = "value" + std::to_string(i);
                engine.put(key, value);
            } else {
                // Read
                std::string key = "mixed" + std::to_string(i / 2);
                volatile std::string value = engine.get(key);
                (void)value;
            }
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        double ops_per_sec = (NUM_OPS * 1000.0) / duration.count();
        
        std::cout << "  Operations: " << NUM_OPS << std::endl;
        std::cout << "  Time: " << duration.count() << " ms" << std::endl;
        std::cout << "  Throughput: " << std::fixed << std::setprecision(2) 
                  << ops_per_sec << " ops/sec" << std::endl << std::endl;
        
        engine.close();
    }
    
    void benchmark_scan() {
        std::cout << "Benchmark: Prefix Scan" << std::endl;
        
        KvEngine engine("./bench_data");
        engine.open();
        
        // Prepare data with prefixes
        const int NUM_PREFIXES = 10;
        const int KEYS_PER_PREFIX = 1000;
        
        for (int p = 0; p < NUM_PREFIXES; ++p) {
            for (int k = 0; k < KEYS_PER_PREFIX; ++k) {
                std::string key = "prefix" + std::to_string(p) + ":key" + std::to_string(k);
                std::string value = "value" + std::to_string(k);
                engine.put(key, value);
            }
        }
        
        // Scan benchmark
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int p = 0; p < NUM_PREFIXES; ++p) {
            std::string prefix = "prefix" + std::to_string(p) + ":";
            auto iterator = engine.scan(prefix);
            
            int count = 0;
            while (iterator->valid()) {
                volatile std::string key = iterator->key();
                volatile std::string value = iterator->value();
                (void)key;
                (void)value;
                count++;
                iterator->next();
            }
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        int total_scanned = NUM_PREFIXES * KEYS_PER_PREFIX;
        double keys_per_sec = (total_scanned * 1000.0) / duration.count();
        
        std::cout << "  Prefixes scanned: " << NUM_PREFIXES << std::endl;
        std::cout << "  Total keys scanned: " << total_scanned << std::endl;
        std::cout << "  Time: " << duration.count() << " ms" << std::endl;
        std::cout << "  Throughput: " << std::fixed << std::setprecision(2) 
                  << keys_per_sec << " keys/sec" << std::endl << std::endl;
        
        engine.close();
    }
};

int main() {
    try {
        Benchmark bench;
        bench.run_all();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Benchmark failed: " << e.what() << std::endl;
        return 1;
    }
}
