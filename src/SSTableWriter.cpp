#include "SSTableWriter.h"

const char MAGIC[] = "SST1";

SSTableWriter::SSTableWriter(const std::string& file) : filename(file) {}

bool SSTableWriter::open() {
    out.open(filename, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) return false;
    
    out.write(MAGIC, 4);
    uint32_t placeholder = 0;
    out.write(reinterpret_cast<const char*>(&placeholder), sizeof(placeholder));
    entryCount = 0;
    return true;
}

bool SSTableWriter::append(const std::string& key, const std::string& value) {
    if (!out.is_open()) return false;

    uint32_t keySize = key.size();
    uint32_t valueSize = value.size();

    out.write(reinterpret_cast<const char*>(&keySize), sizeof(keySize));
    out.write(reinterpret_cast<const char*>(&valueSize), sizeof(valueSize));
    out.write(key.data(), keySize);
    out.write(value.data(), valueSize);

    entryCount++;
    return true;
}

void SSTableWriter::close() {
    if (out.is_open()) {
        out.seekp(4);
        out.write(reinterpret_cast<const char*>(&entryCount), sizeof(entryCount));
        out.close();
    }
}

SSTableWriter::~SSTableWriter() {
    close();
}
