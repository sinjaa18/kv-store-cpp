#include "KVStore.h"
#include <filesystem>
#include <algorithm>
#include <iostream>
#include <ctime>

const size_t MEMTABLE_FLUSH_LIMIT = 100;
const uint32_t MAX_KEY_SIZE = 1024;
const uint32_t MAX_VALUE_SIZE = 1024 * 1024;

enum Operation : uint8_t {
    PUT = 0,
    DELETE = 1
};

KVStore::KVStore() : wal("data/kv.log"), blockCache(std::make_shared<BlockCache>(64)) {
    if (!std::filesystem::exists("data")) {
        std::filesystem::create_directory("data");
    }
    
    loadSSTables();
    
    std::sort(sstables.begin(), sstables.end(), [](const auto& a, const auto& b) {
        return a->getFilename() > b->getFilename();
    });
    
    replay();
}

void KVStore::flushMemTable() {
    if (memtable.size() == 0) return;
    
    std::string ts = std::to_string(std::time(nullptr));
    std::string sstName = "data/sst_" + ts + "_" + std::to_string(nextSSTableId++) + ".sst";
    std::string tmpName = sstName + ".tmp";
    
    SSTableWriter writer(tmpName);
    writer.open();
    for (const auto& entry : memtable.entries()) {
        writer.append(entry.seq, entry.key, entry.value);
    }
    writer.close();
    
    std::error_code ec;
    std::filesystem::rename(tmpName, sstName, ec);
    
    auto reader = std::make_shared<SSTableReader>(sstName, blockCache);
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
    std::string tmpName = newSstName + ".tmp";
    
    SSTableWriter writer(tmpName);
    writer.open();
    
    std::vector<SSTableIterator> iterators;
    for (const auto& reader : sstables) {
        iterators.emplace_back(reader.get());
    }
    
    while (true) {
        int minIdx = -1;
        std::string minKey = "";
        
        for (size_t i = 0; i < iterators.size(); ++i) {
            if (iterators[i].isValid()) {
                std::string k = iterators[i].current().key;
                if (minIdx == -1 || k < minKey) {
                    minIdx = i;
                    minKey = k;
                }
            }
        }
        
        if (minIdx == -1) break;
        
        std::optional<std::string> val;
        uint64_t maxSeq = 0;
        
        for (size_t i = 0; i < iterators.size(); ++i) {
            if (iterators[i].isValid() && iterators[i].current().key == minKey) {
                if (iterators[i].current().seq >= maxSeq) {
                    maxSeq = iterators[i].current().seq;
                    val = iterators[i].current().value;
                }
                iterators[i].next();
            }
        }
        
        writer.append(maxSeq, minKey, val);
    }
    
    writer.close();
    
    std::error_code ec;
    std::filesystem::rename(tmpName, newSstName, ec);
    
    for (const auto& reader : sstables) {
        reader->close();
        std::error_code ec;
        std::filesystem::remove(reader->getFilename(), ec);
    }
    
    sstables.clear();
    auto newReader = std::make_shared<SSTableReader>(newSstName, blockCache);
    if (newReader->open()) {
        sstables.push_back(newReader);
    }
}

void KVStore::loadSSTables() {
    uint64_t currentMaxSeq = sequenceNumber.load();
    for (const auto& entry : std::filesystem::directory_iterator("data")) {
        if (entry.path().extension() == ".sst") {
            auto reader = std::make_shared<SSTableReader>(entry.path().string(), blockCache);
            if (reader->open()) {
                sstables.push_back(reader);
                if (reader->getMaxSequence() > currentMaxSeq) {
                    currentMaxSeq = reader->getMaxSequence();
                }
            }
        }
    }
    sequenceNumber.store(currentMaxSeq);
}

bool KVStore::put(const std::string& key, const std::string& value) {
    if (key.empty()) return false;
    std::lock_guard<std::mutex> lock(mtx);
    
    uint64_t seq = ++sequenceNumber;
    if (!wal.appendPut(seq, key, value)) return false;
    
    memtable.put(seq, key, std::make_optional(value));
    
    if (memtable.size() >= MEMTABLE_FLUSH_LIMIT) {
        flushMemTable();
    }
    
    return true;
}

std::optional<std::string> KVStore::get(const std::string& key) {
    std::lock_guard<std::mutex> lock(mtx);
    
    auto memVal = memtable.get(key);
    if (memVal) {
        return memVal->value;
    }
    
    std::optional<KVPair> bestMatch;
    for (const auto& reader : sstables) {
        auto val = reader->get(key);
        if (val) {
            if (!bestMatch || val->seq > bestMatch->seq) {
                bestMatch = val;
            }
        }
    }
    
    if (bestMatch) {
        return bestMatch->value;
    }
    
    return std::nullopt;
}

bool KVStore::remove(const std::string& key) {
    std::lock_guard<std::mutex> lock(mtx);

    uint64_t seq = ++sequenceNumber;
    if (!wal.appendDelete(seq, key))
        return false;

    memtable.put(seq, key, std::nullopt);
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
            memtable.put(sequence, key, value);
        else
            memtable.put(sequence, key, std::nullopt);
    }

    sequenceNumber.store(lastSequence);
    
    if (memtable.size() >= MEMTABLE_FLUSH_LIMIT) {
        flushMemTable();
    }
}