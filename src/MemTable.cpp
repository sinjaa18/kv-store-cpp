#include "MemTable.h"

bool MemTable::put(
    const std::string& key,
    const std::string& value) {
    return data.put(key,value);
}

std::optional<std::string>
MemTable::get(
    const std::string& key)const {
    return data.get(key);
}


bool MemTable::remove(
    const std::string& key) {
    return data.remove(key) ;
}

std::vector<std::pair<std::string, std::string>> MemTable::entries() const {
    return data.entries();
}