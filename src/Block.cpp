#include "Block.h"
#include <cstring>

BlockBuilder::BlockBuilder() {
    reset();
}

void BlockBuilder::add(const std::string& key, const std::string& value) {
    uint32_t kSize = key.size();
    uint32_t vSize = value.size();
    
    uint8_t* kPtr = (uint8_t*)&kSize;
    buffer.insert(buffer.end(), kPtr, kPtr + sizeof(uint32_t));
    
    uint8_t* vPtr = (uint8_t*)&vSize;
    buffer.insert(buffer.end(), vPtr, vPtr + sizeof(uint32_t));
    
    buffer.insert(buffer.end(), key.begin(), key.end());
    buffer.insert(buffer.end(), value.begin(), value.end());
    
    entryCount++;
}

void BlockBuilder::reset() {
    buffer.clear();
    buffer.resize(sizeof(uint32_t), 0);
    entryCount = 0;
}

size_t BlockBuilder::size() const {
    return buffer.size();
}

const std::vector<uint8_t>& BlockBuilder::finish() {
    std::memcpy(buffer.data(), &entryCount, sizeof(uint32_t));
    return buffer;
}

bool BlockBuilder::isEmpty() const {
    return entryCount == 0;
}

BlockReader::BlockReader(std::vector<uint8_t> blockData) : data(std::move(blockData)) {}

std::vector<std::pair<std::string, std::string>> BlockReader::readAll() const {
    std::vector<std::pair<std::string, std::string>> entries;
    if (data.size() < sizeof(uint32_t)) return entries;
    
    uint32_t count = 0;
    std::memcpy(&count, data.data(), sizeof(uint32_t));
    
    size_t offset = sizeof(uint32_t);
    for (uint32_t i = 0; i < count; ++i) {
        if (offset + 2 * sizeof(uint32_t) > data.size()) break;
        
        uint32_t kSize = 0, vSize = 0;
        std::memcpy(&kSize, data.data() + offset, sizeof(uint32_t));
        offset += sizeof(uint32_t);
        std::memcpy(&vSize, data.data() + offset, sizeof(uint32_t));
        offset += sizeof(uint32_t);
        
        if (offset + kSize + vSize > data.size()) break;
        
        std::string key((char*)data.data() + offset, kSize);
        offset += kSize;
        std::string value((char*)data.data() + offset, vSize);
        offset += vSize;
        
        entries.push_back({key, value});
    }
    
    return entries;
}
