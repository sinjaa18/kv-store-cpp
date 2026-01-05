#include "KVStore.h"
#include<fstream>

bool KVStore::put(const std::string& key, const std::string& val) {
    if (key.empty()) return false;
    table[key] = val;
    return true;
}

std::optional<std::string> KVStore::get(const std::string& key) const {
    auto it = table.find(key);
    if (it == table.end()) {
        return std::nullopt;
    }
    return it->second;
}

bool KVStore::remove(const std::string& key) {
    return table.erase(key) > 0;
}

void KVStore:: flush(){
    std::ofstream fileout("data.csv");
    for(auto it:table){
        fileout<<it.first<<","<<it.second<<"\n";
    }
    fileout.close();
}

