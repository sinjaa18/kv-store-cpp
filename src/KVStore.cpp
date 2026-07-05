#include "KVStore.h"
#include "WALRecord.h"
#include<fstream>
KVStore::KVStore():wal("data/kv.log"){
    replay();
}

bool KVStore::put(
    const std::string& key,
    const std::string& value) {
    if (key.empty())
        return false;
    std::lock_guard<std::mutex> lock(mtx);
    wal.appendPut(key, value);
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
    wal.appendDelete(key);
    return memtable.remove(key);
}

void KVStore::replay() {
    std::ifstream in( "data/kv.log", std::ios::binary);
    if (!in.is_open())
        return;
    while (true) {
        WALRecord rec;
        if (!in.read((char*)&rec,sizeof(rec)))
            break;
        std::string key(rec.keySize,'\0');
        if (!in.read(
            &key[0],
            rec.keySize))
            break;
        std::string value;
        if (rec.valueSize) {
            value.resize(rec.valueSize);
            if (!in.read(  &value[0], rec.valueSize))
                break;
        }
        if (rec.op == PUT)
            memtable.put(key, value);
        else if (rec.op == DELETE)
            memtable.remove(key);
    }
}