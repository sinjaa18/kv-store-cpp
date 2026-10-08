#include "SSTableWriter.h"
#include <cstring>

const char MAGIC[] = "SST2\0\0\0\0";

SSTableWriter::SSTableWriter(const std::string& file) : filename(file) {}

bool SSTableWriter::open() {
    out.open(filename, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) return false;
    currentOffset = 0;
    return true;
}

void SSTableWriter::flushBlock() {
    if (currentBlock.isEmpty()) return;
    
    auto& data = currentBlock.finish();
    uint64_t size = data.size();
    
    out.write(reinterpret_cast<const char*>(data.data()), size);
    
    blocks.push_back({currentFirstKey, currentOffset, size});
    currentOffset += size;
    
    currentBlock.reset();
}

bool SSTableWriter::append(uint64_t seq, const std::string& key, const std::optional<std::string>& value) {
    if (!out.is_open()) return false;
    
    if (currentBlock.isEmpty()) {
        currentFirstKey = key;
    }
    
    currentBlock.add(seq, key, value);
    if (seq > maxSeq) {
        maxSeq = seq;
    }
    if (currentBlock.size() >= BLOCK_SIZE_LIMIT) {
        flushBlock();
    }
    return true;
}

void SSTableWriter::close() {
    if (!out.is_open()) return;
    
    flushBlock();
    
    uint64_t indexOffset = currentOffset;
    BlockBuilder indexBuilder;
    
    for (const auto& bh : blocks) {
        std::string value(sizeof(uint64_t) * 2, '\0');
        std::memcpy(&value[0], &bh.offset, sizeof(uint64_t));
        std::memcpy(&value[sizeof(uint64_t)], &bh.size, sizeof(uint64_t));
        indexBuilder.add(0, bh.firstKey, value);
    }
    
    auto& indexData = indexBuilder.finish();
    uint64_t indexSize = indexData.size();
    
    if (indexSize > 0) {
        out.write(reinterpret_cast<const char*>(indexData.data()), indexSize);
        currentOffset += indexSize;
    }
    
    out.write(reinterpret_cast<const char*>(&maxSeq), sizeof(uint64_t));
    out.write(reinterpret_cast<const char*>(&indexOffset), sizeof(uint64_t));
    out.write(MAGIC, 8);
    
    out.close();
}

SSTableWriter::~SSTableWriter() {
    close();
}
