#include "KVStore.h"
#include "SSTableWriter.h"
#include "WALRecord.h"
#include <fstream>
#include <iostream>
#include <filesystem>
#include <algorithm>
#include <map>
#include <ctime>

constexpr uint32_t MAX_KEY_SIZE = 1 << 20; 
constexpr uint32_t MAX_VALUE_SIZE = 16 << 20;
constexpr size_t MEMTABLE_FLUSH_LIMIT = 100;

KVStore::KVStore():wal("data/kv.log"){
    std::filesystem::create_directories("data");
    
    // Load existing SSTables
    for (const auto& entry : std::filesystem::directory_iterator("data")) {
        if (entry.path().extension() == ".sst") {
            auto reader = std::make_shared<SSTableReader>(entry.path().string());
            if (reader->open()) {
                sstables.push_back(reader);
            }
        }
    }
    
    std::sort(sstables.begin(), sstables.end(), [](const auto& a, const auto& b) {
        return a->getFilename() > b->getFilename();
    });
    
    replay();
}

void KVStore::flushMemTable() {
    if (memtable.size() == 0) return;
    
    std::string ts = std::to_string(std::time(nullptr));
    std::string sstName = "data/sst_" + ts + "_" + std::to_string(nextSSTableId++) + ".sst";
    
    SSTableWriter writer(sstName);
    writer.open();
    for (const auto& entry : memtable.entries()) {
        writer.append(entry.first, entry.second);
    }
    writer.close();
    
    auto reader = std::make_shared<SSTableReader>(sstName);
    if (reader->open()) {
        sstables.insert(sstables.begin(), reader);
    }
    
    memtable.clear();
    wal.clear();
    
    if (sstables.size() >= 4) {
        compact();
    }
}

void KVStore::compact() {
    if (sstables.size() < 2) return;
    
    std::string ts = std::to_string(std::time(nullptr));
    std::string newSstName = "data/sst_" + ts + "_" + std::to_string(nextSSTableId++) + "_compacted.sst";
    
    SSTableWriter writer(newSstName);
    writer.open();
    
    std::map<std::string, std::string> merged;
    
    for (auto it = sstables.rbegin(); it != sstables.rend(); ++it) {
        for (const auto& entry : (*it)->readAll()) {
            merged[entry.first] = entry.second;
        }
    }
    
    for (const auto& [k, v] : merged) {
        if (v != "[[TOMBSTONE]]") {
            writer.append(k, v);
        }
    }
    writer.close();
    
    for (const auto& reader : sstables) {
        reader->close();
        std::error_code ec;
        std::filesystem::remove(reader->getFilename(), ec);
    }
    
    sstables.clear();
    
    auto newReader = std::make_shared<SSTableReader>(newSstName);
    if (newReader->open()) {
        sstables.push_back(newReader);
    }
}

bool KVStore::put(
    const std::string& key,
    const std::string& value) {
    if (key.empty())
        return false;

    std::lock_guard<std::mutex> lock(mtx);
    if (!wal.appendPut(key, value))
        return false;

    memtable.put(key, value);
    if (memtable.size() >= MEMTABLE_FLUSH_LIMIT) {
        flushMemTable();
    }
    return true;
}

std::optional<std::string>
KVStore::get( const std::string& key)const {
    std::lock_guard<std::mutex> lock(mtx);
    auto val = memtable.get(key);
    if (val) {
        if (val.value() == "[[TOMBSTONE]]") return std::nullopt;
        return val;
    }
    
    for (const auto& reader : sstables) {
        val = reader->get(key);
        if (val) {
            if (val.value() == "[[TOMBSTONE]]") return std::nullopt;
            return val;
        }
    }
    
    return std::nullopt;
}

bool KVStore::remove(
    const std::string& key) {
    std::lock_guard<std::mutex> lock(mtx);

    if (!wal.appendDelete(key))
        return false;

    memtable.put(key, "[[TOMBSTONE]]");
    if (memtable.size() >= MEMTABLE_FLUSH_LIMIT) {
        flushMemTable();
    }
    return true;
}

void KVStore::replay() {
    std::ifstream in("data/kv.log", std::ios::binary);

    if (!in.is_open())
        return;

    uint64_t lastSequence = 0;

    while (true) {
        uint64_t sequence;
        uint8_t operation;
        uint32_t keySize;
        uint32_t valueSize;
        uint32_t checksum;

        if (!in.read(reinterpret_cast<char*>(&sequence), sizeof(sequence)))
            break;

        if (!in.read(reinterpret_cast<char*>(&operation), sizeof(operation)))
            break;

        if (!in.read(reinterpret_cast<char*>(&keySize), sizeof(keySize)))
            break;

        if (!in.read(reinterpret_cast<char*>(&valueSize), sizeof(valueSize)))
            break;

        if (!in.read(reinterpret_cast<char*>(&checksum), sizeof(checksum)))
            break;

        if (sequence <= lastSequence)
            break;

        if (keySize == 0 || keySize > MAX_KEY_SIZE)
            break;

        if (operation != static_cast<uint8_t>(PUT) &&
            operation != static_cast<uint8_t>(DELETE))
            break;

        if (operation == static_cast<uint8_t>(DELETE) && valueSize != 0)
            break;

        if (operation == static_cast<uint8_t>(PUT) &&
            valueSize > MAX_VALUE_SIZE)
            break;

        std::string key(keySize, '\0');

        if (!in.read(&key[0], keySize))
            break;

        std::string value;

        if (valueSize) {
            value.resize(valueSize);

            if (!in.read(&value[0], valueSize))
                break;
        }

        if (!wal.verifyChecksum(checksum, key, value))
            break;

        lastSequence = sequence;

        if (operation == static_cast<uint8_t>(PUT))
            memtable.put(key, value);
        else
            memtable.put(key, "[[TOMBSTONE]]");
    }

    wal.setCurrentSequence(lastSequence);
    
    if (memtable.size() >= MEMTABLE_FLUSH_LIMIT) {
        flushMemTable();
    }
}