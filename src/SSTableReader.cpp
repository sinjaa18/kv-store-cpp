#include "SSTableReader.h"
#include <iostream>
#include <cstring>

const char MAGIC_READER[] = "SST2\0\0\0\0";

SSTableReader::SSTableReader(const std::string& file, std::shared_ptr<BlockCache> cache) : filename(file), cache(cache) {}

bool SSTableReader::open() {
    in.open(filename, std::ios::binary | std::ios::ate);
    if (!in.is_open()) return false;
    
    std::streamsize fileSize = in.tellg();
    if (fileSize < 24) return false;
    
    in.seekg(fileSize - 24);
    in.read(reinterpret_cast<char*>(&maxSeq), sizeof(uint64_t));
    uint64_t indexOffset = 0;
    in.read(reinterpret_cast<char*>(&indexOffset), sizeof(uint64_t));
    
    char magicBuf[8];
    in.read(magicBuf, 8);
    if (std::memcmp(magicBuf, MAGIC_READER, 8) != 0) return false;
    
    if (indexOffset > static_cast<uint64_t>(fileSize - 24)) return false;
    
    uint64_t indexSize = (fileSize - 24) - indexOffset;
    if (indexSize > 0) {
        BlockReader indexReader = readBlock(indexOffset, indexSize);
        auto indexEntries = indexReader.readAll();
        
        for (const auto& entry : indexEntries) {
            BlockHandle bh;
            bh.firstKey = entry.key;
            if (entry.value && entry.value->size() == sizeof(uint64_t) * 2) {
                std::memcpy(&bh.offset, entry.value->data(), sizeof(uint64_t));
                std::memcpy(&bh.size, entry.value->data() + sizeof(uint64_t), sizeof(uint64_t));
                
                if (bh.offset >= static_cast<uint64_t>(fileSize) || 
                    bh.size > static_cast<uint64_t>(fileSize) || 
                    bh.offset + bh.size > indexOffset) {
                    return false;
                }
                
                blocks.push_back(bh);
            }
        }
    }
    
    return true;
}

BlockReader SSTableReader::readBlock(uint64_t offset, uint64_t size) {
    std::vector<uint8_t> data(size);
    in.seekg(offset);
    in.read(reinterpret_cast<char*>(data.data()), size);
    return BlockReader(std::move(data));
}

std::vector<KVPair> SSTableReader::getBlockEntries(uint64_t offset, uint64_t size) {
    if (cache) {
        std::string cacheKey = filename + ":" + std::to_string(offset);
        auto cached = cache->get(cacheKey);
        if (cached) {
            return *cached;
        }
        
        BlockReader reader = readBlock(offset, size);
        auto entries = reader.readAll();
        cache->put(cacheKey, entries);
        return entries;
    }
    
    BlockReader reader = readBlock(offset, size);
    return reader.readAll();
}

std::optional<KVPair> SSTableReader::get(const std::string& targetKey) {
    if (!in.is_open() || blocks.empty()) return std::nullopt;
    
    int candidateIndex = -1;
    for (size_t i = 0; i < blocks.size(); ++i) {
        if (blocks[i].firstKey <= targetKey) {
            candidateIndex = i;
        } else {
            break;
        }
    }
    
    if (candidateIndex == -1) return std::nullopt;
    
    auto entries = getBlockEntries(blocks[candidateIndex].offset, blocks[candidateIndex].size);
    
    for (const auto& entry : entries) {
        if (entry.key == targetKey) {
            return entry;
        }
    }
    
    return std::nullopt;
}

std::vector<KVPair> SSTableReader::readAll() {
    std::vector<KVPair> allEntries;
    if (!in.is_open()) return allEntries;
    
    for (const auto& bh : blocks) {
        auto entries = getBlockEntries(bh.offset, bh.size);
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

void SSTableIterator::loadBlock() {
    if (currentBlockIdx < reader->blocks.size()) {
        currentBlockEntries = reader->getBlockEntries(reader->blocks[currentBlockIdx].offset, reader->blocks[currentBlockIdx].size);
        currentEntryIdx = 0;
    } else {
        currentBlockEntries.clear();
    }
}

SSTableIterator::SSTableIterator(SSTableReader* r) : reader(r), currentBlockIdx(0), currentEntryIdx(0) {
    loadBlock();
}

bool SSTableIterator::isValid() const {
    return !currentBlockEntries.empty();
}

KVPair SSTableIterator::current() const {
    return currentBlockEntries[currentEntryIdx];
}

void SSTableIterator::next() {
    currentEntryIdx++;
    if (currentEntryIdx >= currentBlockEntries.size()) {
        currentBlockIdx++;
        loadBlock();
    }
}
