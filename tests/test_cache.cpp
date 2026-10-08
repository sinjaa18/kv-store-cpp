#include <gtest/gtest.h>
#include "BlockCache.h"
#include "KVStore.h"
#include "SSTableWriter.h"
#include <filesystem>
#include <chrono>

class CacheTest : public ::testing::Test {
protected:
    void SetUp() override {
        std::filesystem::remove_all("data");
        std::filesystem::create_directories("data");
    }
    
    void TearDown() override {
        std::filesystem::remove_all("data");
    }
};

TEST_F(CacheTest, DirectCacheLogic) {
    BlockCache cache(2);
    
    // Empty -> miss
    EXPECT_FALSE(cache.get("b1").has_value());
    EXPECT_EQ(cache.getMisses(), 1);
    
    // First access (put)
    std::vector<KVPair> block1 = {{1, "k1", "v1"}};
    cache.put("b1", block1);
    
    // Second access -> hit
    auto res = cache.get("b1");
    EXPECT_TRUE(res.has_value());
    EXPECT_EQ(cache.getHits(), 1);
    
    // Eviction test (capacity 2)
    std::vector<KVPair> block2 = {{2, "k2", "v2"}};
    std::vector<KVPair> block3 = {{3, "k3", "v3"}};
    
    cache.put("b2", block2);
    cache.put("b3", block3); // This should evict b1 since it's LRU
    
    EXPECT_EQ(cache.getEvictions(), 1);
    
    EXPECT_FALSE(cache.get("b1").has_value()); // b1 evicted
    EXPECT_TRUE(cache.get("b2").has_value());  // b2 hit
    EXPECT_TRUE(cache.get("b3").has_value());  // b3 hit
}

TEST_F(CacheTest, CapacityOne) {
    BlockCache cache(1);
    cache.put("b1", {{1, "k1", "v1"}});
    cache.put("b2", {{2, "k2", "v2"}});
    
    EXPECT_EQ(cache.getEvictions(), 1);
    EXPECT_FALSE(cache.get("b1").has_value());
    EXPECT_TRUE(cache.get("b2").has_value());
}

TEST_F(CacheTest, CapacityZeroDisabled) {
    BlockCache cache(0);
    cache.put("b1", {{1, "k1", "v1"}});
    EXPECT_FALSE(cache.get("b1").has_value());
    EXPECT_EQ(cache.getEvictions(), 0);
}

TEST_F(CacheTest, KVStoreIntegration) {
    KVStore store;
    auto cache = store.getBlockCache();
    
    // Write enough data to flush an SSTable
    for (int i = 0; i < 110; i++) {
        store.put("key" + std::to_string(i), "val");
    }
    
    // The put should have triggered a flush (memtable size > 100)
    // Now we get a key, which should hit the SSTable
    auto val = store.get("key0");
    EXPECT_TRUE(val.has_value());
    
    // First access should be a cache miss but populate the cache
    EXPECT_GT(cache->getMisses(), 0);
    
    size_t missesBefore = cache->getMisses();
    size_t hitsBefore = cache->getHits();
    
    // Second access should be a hit
    val = store.get("key0");
    EXPECT_TRUE(val.has_value());
    EXPECT_EQ(cache->getHits(), hitsBefore + 1);
    EXPECT_EQ(cache->getMisses(), missesBefore);
}

TEST_F(CacheTest, TombstoneInCache) {
    KVStore store;
    store.put("k", "v");
    store.remove("k");
    
    for (int i = 0; i < 110; i++) store.put("fill" + std::to_string(i), "v"); // flush
    
    auto val = store.get("k");
    EXPECT_FALSE(val.has_value());
    
    // Ensure it was cached as a tombstone and doesn't crash
    val = store.get("k");
    EXPECT_FALSE(val.has_value());
}
