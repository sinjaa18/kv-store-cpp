#ifndef SSTABLEREADER_H
#define SSTABLEREADER_H

#include <string>
#include <fstream>
#include <optional>
#include <vector>
#include <utility>
#include <cstdint>

#include "Block.h"

class SSTableReader {
    std::string filename;
    std::ifstream in;
    
    struct BlockHandle {
        std::string firstKey;
        uint64_t offset;
        uint64_t size;
    };
    std::vector<BlockHandle> blocks;
public:
    SSTableReader(const std::string& file);
    ~SSTableReader();

    std::string getFilename() const { return filename; }

    bool open();
    std::optional<std::string> get(const std::string& key);
    
    std::vector<std::pair<std::string, std::string>> readAll();
    void close();
private:
    BlockReader readBlock(uint64_t offset, uint64_t size);
};

#endif
