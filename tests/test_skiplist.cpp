#include <gtest/gtest.h>
#include "SkipList.h"

TEST(SkipListTest, BasicPutAndGet) {
    SkipList list;
    EXPECT_TRUE(list.put(1, "a", "10"));
    EXPECT_TRUE(list.put(2, "b", "20"));
    EXPECT_TRUE(list.put(3, "c", "30"));

    EXPECT_EQ(list.get("a")->value, "10");
    EXPECT_EQ(list.get("b")->value, "20");
    EXPECT_EQ(list.get("c")->value, "30");
    EXPECT_FALSE(list.get("d").has_value());
}

TEST(SkipListTest, UpdateExistingKey) {
    SkipList list;
    list.put(1, "key1", "val1");
    EXPECT_EQ(list.get("key1")->value, "val1");
    list.put(2, "key1", "val2");
    EXPECT_EQ(list.get("key1")->value, "val2");
}

TEST(SkipListTest, RemoveKey) {
    SkipList list;
    list.put(1, "key1", "val1");
    EXPECT_TRUE(list.remove(2, "key1"));
    EXPECT_FALSE(list.get("key1").has_value());
    EXPECT_FALSE(list.remove(3, "key1"));
}

TEST(SkipListTest, OrderedIteration) {
    SkipList list;
    list.put(1, "c", "30");
    list.put(2, "a", "10");
    list.put(3, "b", "20");

    auto entries = list.entries();
    ASSERT_EQ(entries.size(), 3);
    EXPECT_EQ(entries[0].key, "a");
    EXPECT_EQ(entries[1].key, "b");
    EXPECT_EQ(entries[2].key, "c");
}
