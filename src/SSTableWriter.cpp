#include "SSTableWriter.h"

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
    
    blocks.push_back({currentOffset, size});
    currentOffset += size;
    
    currentBlock.reset();
}

bool SSTableWriter::append(const std::string& key, const std::string& value) {
    if (!out.is_open()) return false;
    
    currentBlock.add(key, value);
    if (currentBlock.size() >= BLOCK_SIZE_LIMIT) {
        flushBlock();
    }
    return true;
}

void SSTableWriter::close() {
    if (!out.is_open()) return;
    
    flushBlock();
    
    uint64_t indexOffset = currentOffset;
    
    uint64_t indexSize = blocks.size() * sizeof(BlockHandle);
    if (indexSize > 0) {
        out.write(reinterpret_cast<const char*>(blocks.data()), indexSize);
        currentOffset += indexSize;
    }
    
    out.write(reinterpret_cast<const char*>(&indexOffset), sizeof(uint64_t));
    out.write(MAGIC, 8);
    
    out.close();
}

SSTableWriter::~SSTableWriter() {
    close();
}
