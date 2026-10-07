#include "WAL.h"
#include<fstream>

constexpr uint32_t MAX_KEY_SIZE = 1 << 20;
constexpr uint32_t MAX_VALUE_SIZE = 16 << 20;

WAL::WAL(std::string file)
:filename(file){}

void WAL::writeUint64(std::ofstream& out, uint64_t value) {
    out.write(reinterpret_cast<const char*>(&value), sizeof(value));
}

void WAL::writeUint32(std::ofstream& out, uint32_t value) {
    out.write(reinterpret_cast<const char*>(&value), sizeof(value));
}

void WAL::writeUint8(std::ofstream& out, uint8_t value) {
    out.write(reinterpret_cast<const char*>(&value), sizeof(value));
}

uint32_t WAL::checksum(
const std::string& key,
const std::string& value)const{
    uint32_t hash=0;
    for(char c:key)
        hash=hash*31+c;
    for(char c:value)
        hash=hash*31+c;
    return hash;
}

bool WAL::verifyChecksum(
    uint32_t expected,
    const std::string& key,
    const std::string& value
)const {

    return checksum(key, value) == expected;
}

bool WAL::appendPut(
    const std::string& key,
    const std::string& value) {

    std::ofstream out(
        filename,
        std::ios::binary | std::ios::app
    );

    if (!out.is_open())
        return false;
    if (key.empty() || key.size() > MAX_KEY_SIZE)
        return false;

    if (value.size() > MAX_VALUE_SIZE)
        return false;
    uint64_t nextSequence = currentSequence + 1;
    uint32_t keySize = key.size();
    uint32_t valueSize = value.size();
    uint32_t sum = checksum(key, value);

    writeUint64(out, nextSequence);
    writeUint8(out, static_cast<uint8_t>(PUT));
    writeUint32(out, keySize);
    writeUint32(out, valueSize);
    writeUint32(out, sum);

    out.write(key.data(), keySize);
    out.write(value.data(), valueSize);

    out.flush();
    if (!out)
        return false;
    currentSequence = nextSequence;

    return true;
}

bool WAL::appendDelete(
    const std::string& key) {

    std::ofstream out(
        filename,
        std::ios::binary | std::ios::app
    );

    if (!out.is_open())
        return false;
    if (key.empty() || key.size() > MAX_KEY_SIZE)
        return false;

    uint64_t nextSequence = currentSequence + 1;
    uint32_t keySize = key.size();
    uint32_t valueSize = 0;
    uint32_t sum = checksum(key, "");

    writeUint64(out, nextSequence);
    writeUint8(out, static_cast<uint8_t>(DELETE));
    writeUint32(out, keySize);
    writeUint32(out, valueSize);
    writeUint32(out, sum);

    out.write(key.data(), keySize);

    out.flush();

    if (!out)
        return false;

    currentSequence = nextSequence;

    return true;
}

void WAL::setCurrentSequence(uint64_t sequence) {
    currentSequence = sequence;
}

void WAL::clear() {
    std::ofstream out(filename, std::ios::trunc | std::ios::binary);
    out.close();
    currentSequence = 0;
}