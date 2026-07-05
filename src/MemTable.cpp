#include "MemTable.h"

bool MemTable::put(
    const std::string& key,
    const std::string& value) {
    data[key] = value;
    return true;
}

std::optional<std::string>
MemTable::get(
    const std::string& key)const {
    auto it = data.find(key);
    if (it == data.end())
        return std::nullopt;
    return it->second;
}


bool MemTable::remove(
    const std::string& key) {
    return data.erase(key) > 0;
}