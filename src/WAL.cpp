#include "WAL.h"
#include <iostream>
#include <vector>

enum Operation : uint8_t {
    PUT = 0,
    DELETE = 1
};

WAL::WAL(const std::string& path) : filename(path) {
    out.open(filename, std::ios::app | std::ios::binary);
}

WAL::~WAL() {
    if (out.is_open()) out.close();
}

bool WAL::appendPut(uint64_t seq, const std::string& key, const std::string& value) {
    if (!out.is_open()) return false;
    
    uint8_t op = PUT;
    uint32_t kSize = key.size();
    uint32_t vSize = value.size();
    uint32_t checksum = calculateChecksum(key, value);
    
    out.write(reinterpret_cast<const char*>(&seq), sizeof(seq));
    out.write(reinterpret_cast<const char*>(&op), sizeof(op));
    out.write(reinterpret_cast<const char*>(&kSize), sizeof(kSize));
    out.write(reinterpret_cast<const char*>(&vSize), sizeof(vSize));
    out.write(reinterpret_cast<const char*>(&checksum), sizeof(checksum));
    out.write(key.data(), kSize);
    out.write(value.data(), vSize);
    
    out.flush();
    return true;
}

bool WAL::appendDelete(uint64_t seq, const std::string& key) {
    if (!out.is_open()) return false;

    uint8_t op = DELETE;
    uint32_t kSize = key.size();
    uint32_t vSize = 0;
    uint32_t checksum = calculateChecksum(key, "");

    out.write(reinterpret_cast<const char*>(&seq), sizeof(seq));
    out.write(reinterpret_cast<const char*>(&op), sizeof(op));
    out.write(reinterpret_cast<const char*>(&kSize), sizeof(kSize));
    out.write(reinterpret_cast<const char*>(&vSize), sizeof(vSize));
    out.write(reinterpret_cast<const char*>(&checksum), sizeof(checksum));
    out.write(key.data(), kSize);

    out.flush();
    return true;
}

void WAL::clear() {
    if (out.is_open()) out.close();
    out.open(filename, std::ios::trunc | std::ios::binary);
}

uint32_t WAL::calculateChecksum(const std::string& key, const std::string& value) const {
    uint32_t crc = 0xFFFFFFFF;
    auto update = [&crc](const std::string& str) {
        for (char c : str) {
            crc ^= static_cast<uint8_t>(c);
            for (int i = 0; i < 8; i++) {
                crc = (crc >> 1) ^ (0xEDB88320 & (-(crc & 1)));
            }
        }
    };
    update(key);
    update(value);
    return ~crc;
}

bool WAL::verifyChecksum(uint32_t checksum, const std::string& key, const std::string& value) const {
    return calculateChecksum(key, value) == checksum;
}