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

TEST_F(SequenceTest, EmptyWALDoesNotResetSSTableSequence) {
    {
        SSTableWriter w("data/sst_manual.sst");
        w.open();
        w.append(100, "key", "old_value");
        w.close();
    }

    // WAL is empty. Restart must preserve SSTable max sequence.
    {
        KVStore store;
        store.put("key", "new_value");
    }

    // Restart and verify the newer value wins over seq=100.
    {
        KVStore store;
        EXPECT_EQ(store.get("key"), "new_value");
    }
}
TEST_F(SequenceTest, TombstoneSurvivesCompactionAndRestart) {
    {
        KVStore store;

        // SSTable 1: target = old_value
        store.put("target", "old_value");
        for (int i = 0; i < 99; ++i) {
            store.put("fill1_" + std::to_string(i), "value");
        }

        // SSTable 2: tombstone for target
        store.remove("target");
        for (int i = 0; i < 99; ++i) {
            store.put("fill2_" + std::to_string(i), "value");
        }

        // SSTable 3
        for (int i = 0; i < 100; ++i) {
            store.put("fill3_" + std::to_string(i), "value");
        }

        // SSTable 4 -> triggers compaction
        for (int i = 0; i < 100; ++i) {
            store.put("fill4_" + std::to_string(i), "value");
        }

        // Tombstone must still win after compaction.
        EXPECT_FALSE(store.get("target").has_value());
    }

    // Verify the compacted state survives restart.
    {
        KVStore store;
        EXPECT_FALSE(store.get("target").has_value());
    }
}
TEST_F(SequenceTest, NewerValueSurvivesCompactionOverTombstone) {
    {
        KVStore store;

        // SSTable 1: create and delete target.
        store.put("target", "old_value");
        for (int i = 0; i < 99; ++i) {
            store.put("fill1_" + std::to_string(i), "value");
        }

        store.remove("target");
        for (int i = 0; i < 99; ++i) {
            store.put("fill2_" + std::to_string(i), "value");
        }

        // SSTable 3: newer value for target.
        store.put("target", "new_value");
        for (int i = 0; i < 99; ++i) {
            store.put("fill3_" + std::to_string(i), "value");
        }

        // SSTable 4 -> triggers compaction.
        for (int i = 0; i < 100; ++i) {
            store.put("fill4_" + std::to_string(i), "value");
        }

        EXPECT_EQ(store.get("target"), "new_value");
    }

    // Verify the compacted result survives restart.
    {
        KVStore store;
        EXPECT_EQ(store.get("target"), "new_value");
    }
}

TEST_F(SequenceTest, CompactionPreservesUnrelatedKeys) {
    {
        KVStore store;

        for (int i = 0; i < 400; ++i) {
            store.put("key_" + std::to_string(i),
                "value_" + std::to_string(i));
        }

        // Compaction should have occurred multiple times.
        EXPECT_EQ(store.get("key_0"), "value_0");
        EXPECT_EQ(store.get("key_100"), "value_100");
        EXPECT_EQ(store.get("key_200"), "value_200");
        EXPECT_EQ(store.get("key_399"), "value_399");
    }

    // Verify the compacted state after restart.
    {
        KVStore store;

        EXPECT_EQ(store.get("key_0"), "value_0");
        EXPECT_EQ(store.get("key_100"), "value_100");
        EXPECT_EQ(store.get("key_200"), "value_200");
        EXPECT_EQ(store.get("key_399"), "value_399");
    }
}