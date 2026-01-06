
#include "KVStore.h"
#include <fstream>   
#include <sstream>
#include<string>    

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

void KVStore::load(){
    std::ifstream inFile("data.csv");
    if (!inFile.is_open()) {
       return;
    }
    std::string line;
    while (std::getline(inFile, line)) {
        std::stringstream ss(line);
        if (line.empty()) continue;
        std::string key, value;
        if (!std::getline(ss, key, ',')) continue;
        if (!std::getline(ss, value)) continue;
        table[key]=value;
    }
    inFile.close();
}

void KVStore:: flush(){
    std::ofstream fileout("data.csv",std::ios::app);
    for(auto it:table){
        fileout<<it.first<<","<<it.second<<"\n";
    }
    fileout.close();
}

