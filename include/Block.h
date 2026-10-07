#ifndef BLOCK_H
#define BLOCK_H

#include <vector>
#include <string>
#include <cstdint>
#include <utility>

class BlockBuilder {
    std::vector<uint8_t> buffer;
    uint32_t entryCount = 0;
public:
    BlockBuilder();
    void add(const std::string& key, const std::string& value);
    void reset();
    size_t size() const;
    const std::vector<uint8_t>& finish();
    bool isEmpty() const;
};

class BlockReader {
    std::vector<uint8_t> data;
public:
    BlockReader(std::vector<uint8_t> blockData);
    std::vector<std::pair<std::string, std::string>> readAll() const;
};

#endif
