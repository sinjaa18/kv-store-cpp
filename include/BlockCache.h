#ifndef BLOCKCACHE_H
#define BLOCKCACHE_H

#include <string>
#include <vector>
#include <list>
#include <unordered_map>
#include <mutex>
#include <optional>
#include "KVPair.h"

class BlockCache {
    size_t capacity;
    std::list<std::string> lruList;
    
    struct CacheEntry {
        std::vector<KVPair> block;
        std::list<std::string>::iterator it;
    };
    
    std::unordered_map<std::string, CacheEntry> cacheMap;
    
    size_t hits = 0;
    size_t misses = 0;
    size_t evictions = 0;
    std::mutex mtx;

public:
    BlockCache(size_t cap = 64);

    std::optional<std::vector<KVPair>> get(const std::string& key);
    void put(const std::string& key, const std::vector<KVPair>& block);
    
    size_t getHits();
    size_t getMisses();
    size_t getEvictions();
    void setCapacity(size_t cap);
};

#endif
