
#include "KVStore.h"
#include <fstream>   
#include <sstream>
#include<string>    

//putting data
bool KVStore::put(const std::string& key, const std::string& value) {
    if (key.empty()) return false;
    append_put(key,value);
    table[key] = value;
    return true;
}

//retrieving data
std::optional<std::string> KVStore::get(const std::string& key) const {
    auto it = table.find(key);
    if (it == table.end()) {
        return std::nullopt;
    }
    return it->second;
}

//deleting data
bool KVStore::remove(const std::string& key) {
    append_remove(key);
    return table.erase(key) > 0;
}

//CSV FILE

//on start loads all data
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

//on exit save all data
void KVStore:: flush(){
    std::ofstream fileout("data.csv",std::ios::trunc);
    for(auto it:table){
        fileout<<it.first<<","<<it.second<<"\n";
    }
    fileout.close();
}

//BINARY FILE

//binary-append
void KVStore:: append_put(const std::string &key,const std::string &value){
    std::ofstream outFile("data.bin",std::ios::binary|std::ios::app);
    int k = key.size();
    int v = value.size();
    outFile.write((char*)&k,sizeof(int));
    outFile.write((char*)&v,sizeof(int));
    outFile.write(key.c_str(),k);
    outFile.write(value.c_str(),v);
    outFile.close();

}
//binary-tombstone
void KVStore:: append_remove(const std::string &key){
    std::ofstream outFile("data.bin",std::ios::binary|std::ios::app);
    int k = key.size();
    int v = -1;
    outFile.write((char*)&k,sizeof(int));
    outFile.write((char*)&v,sizeof(int));
    outFile.write(key.c_str(),k);
    outFile.close();

}

//binary-replay
void KVStore::replay() {
    std::ifstream inFile("data.bin", std::ios::binary);
    if (!inFile.is_open()) {
        return;
    }

    while (true) {
        int k, v;
        if (!inFile.read(reinterpret_cast<char*>(&k), sizeof(k)))
            break;

        if (!inFile.read(reinterpret_cast<char*>(&v), sizeof(v)))
            break;

        std::string key(k, '\0');

        if (!inFile.read(&key[0], k))
            break;

        if(v==-1){
           if(table.find(key)!=table.end()) table.erase(key);
           continue;
        }
            std::string value(v, '\0');
            if(!inFile.read(&value[0], v))
            break;
            table[key] = value;
    }

    inFile.close();
}
