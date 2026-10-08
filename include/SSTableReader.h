#ifndef SSTABLEREADER_H
#define SSTABLEREADER_H

#include <string>
#include <fstream>
#include <optional>
#include <vector>
#include <utility>
#include <cstdint>

#include "Block.h"
#include "BlockCache.h"
#include <memory>

class SSTableReader {
    std::string filename;
    std::ifstream in;
    
    struct BlockHandle {
        std::string firstKey;
        uint64_t offset;
        uint64_t size;
    };
    std::vector<BlockHandle> blocks;
    uint64_t maxSeq = 0;
    std::shared_ptr<BlockCache> cache;
public:
    SSTableReader(const std::string& file, std::shared_ptr<BlockCache> cache = nullptr);
    ~SSTableReader();

    std::string getFilename() const { return filename; }
    uint64_t getMaxSequence() const { return maxSeq; }

    bool open();
    std::optional<KVPair> get(const std::string& key);
    
    std::vector<KVPair> readAll();
    void close();

    friend class SSTableIterator;
private:
    BlockReader readBlock(uint64_t offset, uint64_t size);
    std::vector<KVPair> getBlockEntries(uint64_t offset, uint64_t size);
};

class SSTableIterator {
    SSTableReader* reader;
    size_t currentBlockIdx;
    size_t currentEntryIdx;
    std::vector<KVPair> currentBlockEntries;

    void loadBlock();
public:
    SSTableIterator(SSTableReader* r);
    bool isValid() const;
    KVPair current() const;
    void next();
};

#endif
