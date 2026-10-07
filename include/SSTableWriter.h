#ifndef SSTABLEWRITER_H
#define SSTABLEWRITER_H

#include <string>
#include <fstream>
#include <vector>
#include <utility>
#include <cstdint>

#include "Block.h"

class SSTableWriter {
    std::string filename;
    std::ofstream out;
    BlockBuilder currentBlock;
    
    struct BlockHandle {
        uint64_t offset;
        uint64_t size;
    };
    std::vector<BlockHandle> blocks;
    uint64_t currentOffset = 0;
    
    static constexpr size_t BLOCK_SIZE_LIMIT = 4096;

    void flushBlock();
public:
    SSTableWriter(const std::string& file);
    ~SSTableWriter();

    bool open();
    bool append(const std::string& key, const std::string& value);
    void close();
};

#endif
