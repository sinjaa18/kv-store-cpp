#include <gtest/gtest.h>
#include "SSTableReader.h"
#include "SSTableWriter.h"
#include <fstream>
#include <filesystem>
#include <cstring>

TEST(CorruptionTest, InvalidFooter) {
    std::string filename = "data/corrupt_footer.sst";
    {
        std::ofstream out(filename, std::ios::binary);
        out.write("garbage data here that is more than 16 bytes long!!", 50);
    }
    SSTableReader reader(filename);
    EXPECT_FALSE(reader.open());
}

TEST(CorruptionTest, TruncatedFile) {
    std::string filename = "data/truncated.sst";
    {
        std::ofstream out(filename, std::ios::binary);
        out.write("short", 5);
    }
    SSTableReader reader(filename);
    EXPECT_FALSE(reader.open());
}

TEST(CorruptionTest, InvalidIndexOffset) {
    std::string filename = "data/bad_offset.sst";
    {
        SSTableWriter writer(filename);
        writer.open();
        writer.append(1, "key1", "val1");
        writer.close();
        
        std::fstream f(filename, std::ios::in | std::ios::out | std::ios::binary);
        f.seekp(-16, std::ios::end);
        uint64_t badOffset = 0xFFFFFFFFFFFFFFFF;
        f.write(reinterpret_cast<char*>(&badOffset), sizeof(uint64_t));
    }
    SSTableReader reader(filename);
    EXPECT_FALSE(reader.open());
}
