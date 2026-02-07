#include "KVStore.h"
#include <fstream>
#include <sstream>
#include <string>

// PUT
bool KVStore::put(const std::string& key, const std::string& value) {
    if (key.empty()) return false;

    std::lock_guard<std::mutex> lock(mtx);   //lock

    append_put(key, value);
    table[key] = value;

    return true;
}

// GET
std::optional<std::string> KVStore::get(const std::string& key) const {
    std::lock_guard<std::mutex> lock(mtx);   //lock

    auto it = table.find(key);
    if (it == table.end())
        return std::nullopt;

    return it->second;
}

// REMOVE
bool KVStore::remove(const std::string& key) {
    std::lock_guard<std::mutex> lock(mtx);

    append_remove(key);
    return table.erase(key) > 0;
}

// ---------------- BINARY FILE ----------------

// append PUT
void KVStore::append_put(const std::string &key,const std::string &value){
    std::ofstream outFile("data.bin", std::ios::binary | std::ios::app);

    int k = key.size();
    int v = value.size();

    outFile.write(reinterpret_cast<char*>(&k), sizeof(int));
    outFile.write(reinterpret_cast<char*>(&v), sizeof(int));
    outFile.write(key.c_str(), k);
    outFile.write(value.c_str(), v);

    outFile.close();
}

// append DELETE (tombstone)
void KVStore::append_remove(const std::string &key){
    std::ofstream outFile("data.bin", std::ios::binary | std::ios::app);

    int k = key.size();
    int v = -1;   // tombstone marker

    outFile.write(reinterpret_cast<char*>(&k), sizeof(int));
    outFile.write(reinterpret_cast<char*>(&v), sizeof(int));
    outFile.write(key.c_str(), k);

    outFile.close();
}

// replay log
void KVStore::replay() {
    std::lock_guard<std::mutex> lock(mtx);   // protect during startup

    std::ifstream inFile("data.bin", std::ios::binary);
    if (!inFile.is_open())
        return;
    while (true) {
        int k, v;
        if (!inFile.read(reinterpret_cast<char*>(&k), sizeof(k)))
            break;
        if (!inFile.read(reinterpret_cast<char*>(&v), sizeof(v)))
            break;
        std::string key(k, '\0');
        if (!inFile.read(&key[0], k))
            break;
        if (v == -1) {
            table.erase(key);
            continue;
        }

        std::string value(v, '\0');
        if (!inFile.read(&value[0], v))
            break;
        table[key] = value;
    }

    inFile.close();
}
