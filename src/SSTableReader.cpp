#include "SSTableReader.h"
#include <iostream>
#include <cstring>

const char MAGIC_READER[] = "SST2\0\0\0\0";

SSTableReader::SSTableReader(const std::string& file) : filename(file) {}

bool SSTableReader::open() {
    in.open(filename, std::ios::binary | std::ios::ate);
    if (!in.is_open()) return false;
    
    std::streamsize fileSize = in.tellg();
    if (fileSize < 16) return false;
    
    in.seekg(fileSize - 16);
    uint64_t indexOffset = 0;
    in.read(reinterpret_cast<char*>(&indexOffset), sizeof(uint64_t));
    
    char magicBuf[8];
    in.read(magicBuf, 8);
    if (std::memcmp(magicBuf, MAGIC_READER, 8) != 0) return false;
    
    if (indexOffset > static_cast<uint64_t>(fileSize - 16)) return false;
    
    uint64_t indexSize = (fileSize - 16) - indexOffset;
    if (indexSize % sizeof(BlockHandle) != 0) return false;
    
    uint64_t numBlocks = indexSize / sizeof(BlockHandle);
    blocks.resize(numBlocks);
    
    if (numBlocks > 0) {
        in.seekg(indexOffset);
        in.read(reinterpret_cast<char*>(blocks.data()), indexSize);
    }
    
    return true;
}

BlockReader SSTableReader::readBlock(uint64_t offset, uint64_t size) {
    std::vector<uint8_t> data(size);
    in.seekg(offset);
    in.read(reinterpret_cast<char*>(data.data()), size);
    return BlockReader(std::move(data));
}

std::optional<std::string> SSTableReader::get(const std::string& targetKey) {
    if (!in.is_open()) return std::nullopt;
    
    for (const auto& bh : blocks) {
        BlockReader reader = readBlock(bh.offset, bh.size);
        auto entries = reader.readAll();
        for (const auto& entry : entries) {
            if (entry.first == targetKey) {
                return entry.second;
            }
        }
    }
    
    return std::nullopt;
}

std::vector<std::pair<std::string, std::string>> SSTableReader::readAll() {
    std::vector<std::pair<std::string, std::string>> allEntries;
    if (!in.is_open()) return allEntries;
    
    for (const auto& bh : blocks) {
        BlockReader reader = readBlock(bh.offset, bh.size);
        auto entries = reader.readAll();
        allEntries.insert(allEntries.end(), entries.begin(), entries.end());
    }
    return allEntries;
}

void SSTableReader::close() {
    if (in.is_open()) in.close();
}

SSTableReader::~SSTableReader() {
    close();
}
