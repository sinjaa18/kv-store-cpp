#include <gtest/gtest.h>
#include "SSTableWriter.h"
#include "SSTableReader.h"
#include <filesystem>

class SSTableTest : public ::testing::Test {
protected:
    std::string testFile = "test_sstable.sst";
    
    void SetUp() override {
        std::filesystem::remove(testFile);
    }
    
    void TearDown() override {
        std::filesystem::remove(testFile);
    }
};

TEST_F(SSTableTest, WriteAndRead) {
    {
        SSTableWriter writer(testFile);
        EXPECT_TRUE(writer.open());
        EXPECT_TRUE(writer.append(1, "key1", "value1"));
        EXPECT_TRUE(writer.append(2, "key2", "value2"));
        EXPECT_TRUE(writer.append(3, "key3", "value3"));
        writer.close();
    }

    {
        SSTableReader reader(testFile);
        EXPECT_TRUE(reader.open());
        
        EXPECT_EQ(reader.get("key2")->value, "value2");
        EXPECT_EQ(reader.get("key3")->value, "value3");
        EXPECT_EQ(reader.get("key1")->value, "value1");
        EXPECT_FALSE(reader.get("key4").has_value());
        
        auto all = reader.readAll();
        ASSERT_EQ(all.size(), 3);
        EXPECT_EQ(all[0].key, "key1");
        EXPECT_EQ(all[2].key, "key3");
        reader.close();
    }
}
