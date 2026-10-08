#include <gtest/gtest.h>
#include "KVStore.h"
#include "SSTableWriter.h"
#include <filesystem>
#include <fstream>
#include <thread>
#include <chrono>

class SequenceTest : public ::testing::Test {
protected:
    void SetUp() override {
        std::filesystem::remove_all("data");
        std::filesystem::create_directories("data");
    }
    
    void TearDown() override {
        std::filesystem::remove_all("data");
    }
};

// Phase 6: VERSION ORDERING TESTS
TEST_F(SequenceTest, VersionOrderingOverridesFilesystem) {
    // Manually create two SSTables
    {
        SSTableWriter w1("data/sst_1.sst");
        w1.open();
        w1.append(100, "a", "NEW"); // High seq number
        w1.close();
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    {
        SSTableWriter w2("data/sst_2.sst");
        w2.open();
        w2.append(50, "a", "OLD"); // Lower seq number, but written LATER
        w2.close();
    }
    
    KVStore store;
    // The database must pick "NEW" because 100 > 50, regardless of file order
    auto val = store.get("a");
    EXPECT_TRUE(val.has_value());
    EXPECT_EQ(*val, "NEW");
}

TEST_F(SequenceTest, VersionOrderingReversedFilesystem) {
    {
        SSTableWriter w1("data/sst_1.sst");
        w1.open();
        w1.append(50, "a", "OLD"); 
        w1.close();
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    {
        SSTableWriter w2("data/sst_2.sst");
        w2.open();
        w2.append(100, "a", "NEW"); 
        w2.close();
    }
    
    KVStore store;
    auto val = store.get("a");
    EXPECT_TRUE(val.has_value());
    EXPECT_EQ(*val, "NEW");
}

// Phase 8: LITERAL TOMBSTONE VALUE
TEST_F(SequenceTest, LiteralTombstoneWorks) {
    {
        KVStore store;
        store.put("a", "[[TOMBSTONE]]");
        EXPECT_EQ(store.get("a"), "[[TOMBSTONE]]");
        
        store.remove("a");
        EXPECT_FALSE(store.get("a").has_value());
    }
    
    // Restart
    {
        KVStore store;
        EXPECT_FALSE(store.get("a").has_value());
    }
}

// Phase 7: TOMBSTONE + SEQUENCE TESTS
TEST_F(SequenceTest, TombstoneOverridesPreviousValue) {
    {
        KVStore store;
        store.put("a", "hello");
        store.remove("a");
        EXPECT_FALSE(store.get("a").has_value());
    }
    
    // Restart
    {
        KVStore store;
        EXPECT_FALSE(store.get("a").has_value());
    }
}

TEST_F(SequenceTest, TombstoneCanBeOverwritten) {
    {
        KVStore store;
        store.put("a", "hello");
        store.remove("a");
        store.put("a", "world");
        EXPECT_EQ(store.get("a"), "world");
    }
    
    {
        KVStore store;
        EXPECT_EQ(store.get("a"), "world");
    }
}

TEST_F(SequenceTest, RestartSequenceIsHigherThanSSTable) {
    {
        KVStore store;
        store.put("key", "mem_value1");
    }
    
    // Write an SSTable with high sequence manually
    {
        SSTableWriter w("data/sst_manual.sst");
        w.open();
        w.append(100, "high_seq_key", "old_value");
        w.close();
    }
    
    {
        // When store starts, it should see maxSeq = 100
        KVStore store;
        // This put should get seq = 101
        store.put("high_seq_key", "new_value");
        
        EXPECT_EQ(store.get("high_seq_key"), "new_value"); // 101 > 100
    }
}
