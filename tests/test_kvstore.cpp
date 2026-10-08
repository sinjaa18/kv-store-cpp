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

TEST_F(KVStoreTest, TruncatedFinalWALRecordIsIgnored) {
    {
        KVStore store;
        store.put("k1", "v1");
        store.put("k2", "v2");
    }

    // Truncate the final WAL record so it becomes incomplete.
    auto size = std::filesystem::file_size(logFile);
    ASSERT_GT(size, 0);

    std::filesystem::resize_file(logFile, size - 1);

    {
        KVStore store;

        // The first complete record must still be recovered.
        EXPECT_EQ(store.get("k1"), "v1");

        // The partially written final record must not be replayed.
        EXPECT_FALSE(store.get("k2").has_value());
    }
}
TEST_F(KVStoreTest, CorruptedWALRecordIsIgnored) {
    {
        KVStore store;
        store.put("k1", "v1");
    }

    // Corrupt the checksum of the first WAL record.
    {
        std::fstream file(
            logFile,
            std::ios::in | std::ios::out | std::ios::binary
        );

        ASSERT_TRUE(file.is_open());

        // WAL layout:
        // uint64_t sequence
        // uint8_t operation
        // uint32_t keySize
        // uint32_t valueSize
        // uint32_t checksum
        //
        // Checksum begins at byte offset 17.
        file.seekp(17);
        uint32_t corruptedChecksum = 0;
        file.write(
            reinterpret_cast<const char*>(&corruptedChecksum),
            sizeof(corruptedChecksum)
        );
    }

    {
        KVStore store;
        EXPECT_FALSE(store.get("k1").has_value());
    }
}
TEST_F(KVStoreTest, ValidWALRecordsBeforeCorruptionAreRecovered) {
    {
        KVStore store;
        store.put("k1", "v1");
        store.put("k2", "v2");
    }

    // Corrupt the second WAL record's checksum.
    {
        std::fstream file(
            logFile,
            std::ios::in | std::ios::out | std::ios::binary
        );

        ASSERT_TRUE(file.is_open());

        // First record size:
        // 8 seq + 1 op + 4 keySize + 4 valueSize + 4 checksum
        // + 2 ("k1") + 2 ("v1") = 25 bytes
        constexpr std::streamoff secondChecksumOffset = 25 + 17;

        file.seekp(secondChecksumOffset);

        uint32_t corruptedChecksum = 0;
        file.write(
            reinterpret_cast<const char*>(&corruptedChecksum),
            sizeof(corruptedChecksum)
        );
    }

    {
        KVStore store;

        EXPECT_EQ(store.get("k1"), "v1");
        EXPECT_FALSE(store.get("k2").has_value());
    }
}