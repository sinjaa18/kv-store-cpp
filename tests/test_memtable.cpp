#include <gtest/gtest.h>
#include "MemTable.h"

TEST(MemTableTest, BasicOperations) {
    MemTable table;
    EXPECT_TRUE(table.put("key1", "val1"));
    EXPECT_EQ(table.get("key1"), "val1");
    EXPECT_TRUE(table.remove("key1"));
    EXPECT_FALSE(table.get("key1").has_value());
}

TEST(MemTableTest, OrderedIteration) {
    MemTable table;
    table.put("b", "20");
    table.put("a", "10");
    table.put("c", "30");

    auto entries = table.entries();
    ASSERT_EQ(entries.size(), 3);
    EXPECT_EQ(entries[0].first, "a");
    EXPECT_EQ(entries[1].first, "b");
    EXPECT_EQ(entries[2].first, "c");
}
