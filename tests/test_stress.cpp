#include "KVStore.h"
#include <iostream>
#include <cassert>
#include <string>
#include <filesystem>

void runStressTest() {
    std::filesystem::remove_all("data");
    std::filesystem::create_directories("data");
    
    std::cout << "Starting Stress Test...\n";
    
    const int NUM_KEYS = 20000;
    
    {
        KVStore store;
        
        // PUT
        for (int i = 0; i < NUM_KEYS; i++) {
            store.put("key" + std::to_string(i), "val" + std::to_string(i));
        }
        
        // GET
        auto v = store.get("key1234");
        assert(v.has_value() && *v == "val1234");
        
        // DELETE half
        for (int i = 0; i < NUM_KEYS; i += 2) {
            store.remove("key" + std::to_string(i));
        }
        
        // GET after delete
        assert(!store.get("key0").has_value());
        assert(store.get("key1").has_value());
    }
    
    // Restart
    {
        KVStore store;
        // Verify final state
        assert(!store.get("key0").has_value());
        assert(store.get("key1").has_value());
        
        auto v = store.get("key19999");
        assert(v.has_value() && *v == "val19999");
        
        assert(!store.get("key19998").has_value());
    }
    
    std::cout << "Stress Test Complete: All final states correct.\n";
}

int main() {
    runStressTest();
    return 0;
}
