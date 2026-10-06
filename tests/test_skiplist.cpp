#include <gtest/gtest.h>
#include "SkipList.h"

TEST(SkipListTest, BasicPutAndGet) {
    SkipList list;
    EXPECT_TRUE(list.put("a", "10"));
    EXPECT_TRUE(list.put("b", "20"));
    EXPECT_TRUE(list.put("c", "30"));

    EXPECT_EQ(list.get("a"), "10");
    EXPECT_EQ(list.get("b"), "20");
    EXPECT_EQ(list.get("c"), "30");
    EXPECT_FALSE(list.get("d").has_value());
}

TEST(SkipListTest, UpdateExistingKey) {
    SkipList list;
    list.put("key1", "val1");
    EXPECT_EQ(list.get("key1"), "val1");
    list.put("key1", "val2");
    EXPECT_EQ(list.get("key1"), "val2");
}

TEST(SkipListTest, RemoveKey) {
    SkipList list;
    list.put("key1", "val1");
    EXPECT_TRUE(list.remove("key1"));
    EXPECT_FALSE(list.get("key1").has_value());
    EXPECT_FALSE(list.remove("key1")); // Removing already removed key
}

TEST(SkipListTest, OrderedIteration) {
    SkipList list;
    list.put("c", "30");
    list.put("a", "10");
    list.put("b", "20");

    auto entries = list.entries();
    ASSERT_EQ(entries.size(), 3);
    EXPECT_EQ(entries[0].first, "a");
    EXPECT_EQ(entries[1].first, "b");
    EXPECT_EQ(entries[2].first, "c");
}
