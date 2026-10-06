#include <gtest/gtest.h>
#include "KVStore.h"
#include <filesystem>
#include <fstream>

class KVStoreTest : public ::testing::Test {
protected:
    std::string logFile = "data/kv.log";
    
    void SetUp() override {
        std::filesystem::create_directories("data");
        std::filesystem::remove(logFile);
    }
    
    void TearDown() override {
        std::filesystem::remove(logFile);
    }
};

TEST_F(KVStoreTest, BasicPutGetRemove) {
    KVStore store;
    EXPECT_TRUE(store.put("k1", "v1"));
    EXPECT_EQ(store.get("k1"), "v1");
    EXPECT_TRUE(store.remove("k1"));
    EXPECT_FALSE(store.get("k1").has_value());
}

TEST_F(KVStoreTest, CrashRecovery) {
    {
        KVStore store;
        store.put("k1", "v1");
        store.put("k2", "v2");
        store.remove("k1");
    }
    
    {
        KVStore store2;
        EXPECT_FALSE(store2.get("k1").has_value());
        EXPECT_EQ(store2.get("k2"), "v2");
    }
}
