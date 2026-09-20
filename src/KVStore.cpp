#include "KVStore.h"
#include "WALRecord.h"
#include<fstream>

constexpr uint32_t MAX_KEY_SIZE = 1 << 20; 
constexpr uint32_t MAX_VALUE_SIZE = 16 << 20;
KVStore::KVStore():wal("data/kv.log"){
    replay();
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
    return true;
}

std::optional<std::string>
KVStore::get( const std::string& key)const {
    std::lock_guard<std::mutex> lock(mtx);
    return memtable.get(key);
}

bool KVStore::remove(
    const std::string& key) {
    std::lock_guard<std::mutex> lock(mtx);

    if (!wal.appendDelete(key))
        return false;
    return memtable.remove(key);
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
            memtable.remove(key);
    }

    wal.setCurrentSequence(lastSequence);
}