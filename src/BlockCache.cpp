#include "BlockCache.h"

BlockCache::BlockCache(size_t cap) : capacity(cap) {}

std::optional<std::vector<KVPair>> BlockCache::get(const std::string& key) {
    std::lock_guard<std::mutex> lock(mtx);
    if (capacity == 0) return std::nullopt;
    
    auto it = cacheMap.find(key);
    if (it == cacheMap.end()) {
        misses++;
        return std::nullopt;
    }
    
    hits++;
    // Move to front (most recently used)
    lruList.erase(it->second.it);
    lruList.push_front(key);
    it->second.it = lruList.begin();
    
    return it->second.block;
}

void BlockCache::put(const std::string& key, const std::vector<KVPair>& block) {
    std::lock_guard<std::mutex> lock(mtx);
    if (capacity == 0) return;
    
    auto it = cacheMap.find(key);
    if (it != cacheMap.end()) {
        // Update existing
        it->second.block = block;
        lruList.erase(it->second.it);
        lruList.push_front(key);
        it->second.it = lruList.begin();
        return;
    }
    
    // Evict if at capacity
    if (cacheMap.size() >= capacity) {
        std::string oldest = lruList.back();
        lruList.pop_back();
        cacheMap.erase(oldest);
        evictions++;
    }
    
    // Insert new
    lruList.push_front(key);
    cacheMap[key] = {block, lruList.begin()};
}

size_t BlockCache::getHits() {
    std::lock_guard<std::mutex> lock(mtx);
    return hits;
}

size_t BlockCache::getMisses() {
    std::lock_guard<std::mutex> lock(mtx);
    return misses;
}

size_t BlockCache::getEvictions() {
    std::lock_guard<std::mutex> lock(mtx);
    return evictions;
}

void BlockCache::setCapacity(size_t cap) {
    std::lock_guard<std::mutex> lock(mtx);
    capacity = cap;
    while (cacheMap.size() > capacity) {
        std::string oldest = lruList.back();
        lruList.pop_back();
        cacheMap.erase(oldest);
        evictions++;
    }
}
