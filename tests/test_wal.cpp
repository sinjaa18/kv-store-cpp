#include <gtest/gtest.h>
#include "WAL.h"
#include <fstream>
#include <filesystem>

class WALTest : public ::testing::Test {
protected:
    std::string testFile = "test_wal.log";
    
    void SetUp() override {
        std::filesystem::remove(testFile);
    }
    
    void TearDown() override {
        std::filesystem::remove(testFile);
    }
};

TEST_F(WALTest, AppendPutAndDelete) {
    WAL wal(testFile);
    EXPECT_TRUE(wal.appendPut(1, "key1", "val1"));
    EXPECT_TRUE(wal.appendDelete(2, "key1"));
    EXPECT_TRUE(std::filesystem::exists(testFile));
    EXPECT_GT(std::filesystem::file_size(testFile), 0);
}
