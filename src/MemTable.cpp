#include "MemTable.h"

bool MemTable::put(
    uint64_t seq,
    const std::string& key,
    const std::optional<std::string>& value) {
    return data.put(seq, key,value);
}

std::optional<KVPair>
MemTable::get(
    const std::string& key)const {
    return data.get(key);
}


bool MemTable::remove(
    uint64_t seq,
    const std::string& key) {
    return data.remove(seq, key) ;
}

std::vector<KVPair> MemTable::entries() const {
    return data.entries();
}

size_t MemTable::size() const {
    return data.size();
}

void MemTable::clear() {
    data.clear();
}