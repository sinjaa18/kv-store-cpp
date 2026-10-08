#include "KVStore.h"
#include <iostream>
#include <chrono>
#include <vector>
#include <string>
#include <filesystem>

void runBenchmark(bool useCache) {
    std::filesystem::remove_all("data");
    std::filesystem::create_directories("data");
    
    long long putTime, randomGetTime, hotGetTime;
    size_t hits = 0, misses = 0, evictions = 0;
    
    {
        KVStore store;
        if (!useCache) {
            store.getBlockCache()->setCapacity(0);
        }
        
        const int NUM_KEYS = 10000;
        
        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < NUM_KEYS; i++) {
            store.put("key" + std::to_string(i), "value" + std::to_string(i));
        }
        auto end = std::chrono::high_resolution_clock::now();
        putTime = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        
        start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < NUM_KEYS; i++) {
            int idx = (i * 17) % NUM_KEYS;
            store.get("key" + std::to_string(idx));
        }
        end = std::chrono::high_resolution_clock::now();
        randomGetTime = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        
        start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < NUM_KEYS; i++) {
            store.get("key100");
        }
        end = std::chrono::high_resolution_clock::now();
        hotGetTime = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        
        if (useCache) {
            auto c = store.getBlockCache();
            hits = c->getHits();
            misses = c->getMisses();
            evictions = c->getEvictions();
        }
    }
    
    std::cout << "--- Cache " << (useCache ? "ON" : "OFF") << " ---\n";
    std::cout << "Sequential PUT: " << putTime << " ms\n";
    std::cout << "Random GET:     " << randomGetTime << " ms\n";
    std::cout << "Hot GET:        " << hotGetTime << " ms\n";
    
    if (useCache) {
        std::cout << "Hits: " << hits << ", Misses: " << misses << ", Evictions: " << evictions << "\n";
    }
    std::cout << "\n";
}

int main() {
    std::cout << "Starting Benchmarks...\n\n";
    runBenchmark(false);
    runBenchmark(true);
    return 0;
}
