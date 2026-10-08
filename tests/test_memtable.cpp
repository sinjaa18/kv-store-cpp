#include <gtest/gtest.h>
#include "MemTable.h"

TEST(MemTableTest, BasicOperations) {
    MemTable table;
    EXPECT_TRUE(table.put(1, "key1", "val1"));
    EXPECT_EQ(table.get("key1")->value, "val1");
    EXPECT_TRUE(table.remove(2, "key1"));
    EXPECT_FALSE(table.get("key1").has_value());
}

TEST(MemTableTest, OrderedIteration) {
    MemTable table;
    table.put(1, "b", "20");
    table.put(2, "a", "10");
    table.put(3, "c", "30");

    auto entries = table.entries();
    ASSERT_EQ(entries.size(), 3);
    EXPECT_EQ(entries[0].key, "a");
    EXPECT_EQ(entries[1].key, "b");
    EXPECT_EQ(entries[2].key, "c");
}
