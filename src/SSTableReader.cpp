#include "SSTableReader.h"

const char MAGIC_READER[] = "SST1";
constexpr uint32_t MAX_KEY_SIZE = 1 << 20;
constexpr uint32_t MAX_VALUE_SIZE = 16 << 20;

SSTableReader::SSTableReader(const std::string& file) : filename(file) {}

bool SSTableReader::open() {
    in.open(filename, std::ios::binary);
    if (!in.is_open()) return false;

    char magicBuf[4];
    if (!in.read(magicBuf, 4) || std::string(magicBuf, 4) != std::string(MAGIC_READER, 4)) {
        return false;
    }

    if (!in.read(reinterpret_cast<char*>(&entryCount), sizeof(entryCount))) {
        return false;
    }

    return true;
}

std::optional<std::string> SSTableReader::get(const std::string& targetKey) {
    if (!in.is_open()) return std::nullopt;

    in.seekg(8);

    for (uint32_t i = 0; i < entryCount; ++i) {
        uint32_t keySize, valueSize;
        if (!in.read(reinterpret_cast<char*>(&keySize), sizeof(keySize))) break;
        if (!in.read(reinterpret_cast<char*>(&valueSize), sizeof(valueSize))) break;

        if (keySize > MAX_KEY_SIZE || valueSize > MAX_VALUE_SIZE) break;

        std::string key(keySize, '\0');
        if (!in.read(&key[0], keySize)) break;

        if (key == targetKey) {
            std::string value(valueSize, '\0');
            if (!in.read(&value[0], valueSize)) break;
            return value;
        } else {
            in.seekg(valueSize, std::ios::cur);
        }
    }
    return std::nullopt;
}

std::vector<std::pair<std::string, std::string>> SSTableReader::readAll() {
    std::vector<std::pair<std::string, std::string>> entries;
    if (!in.is_open()) return entries;

    in.seekg(8);
    for (uint32_t i = 0; i < entryCount; ++i) {
        uint32_t keySize, valueSize;
        if (!in.read(reinterpret_cast<char*>(&keySize), sizeof(keySize))) break;
        if (!in.read(reinterpret_cast<char*>(&valueSize), sizeof(valueSize))) break;

        if (keySize > MAX_KEY_SIZE || valueSize > MAX_VALUE_SIZE) break;

        std::string key(keySize, '\0');
        if (!in.read(&key[0], keySize)) break;

        std::string value(valueSize, '\0');
        if (!in.read(&value[0], valueSize)) break;

        entries.push_back({key, value});
    }
    return entries;
}

void SSTableReader::close() {
    if (in.is_open()) {
        in.close();
    }
}

SSTableReader::~SSTableReader() {
    close();
}
