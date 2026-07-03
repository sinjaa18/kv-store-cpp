#include "KVStore.h"
#include <fstream>

KVStore::KVStore()
:wal("data/kv.log")
{
    replay();
}

bool KVStore::put(const std::string& key,const std::string& value){
    if(key.empty()) return false;

    std::lock_guard<std::mutex> lock(mtx);

    wal.appendPut(key,value);
    table[key]=value;

    return true;
}


std::optional<std::string> KVStore::get(const std::string& key)const{
    std::lock_guard<std::mutex> lock(mtx);

    auto it=table.find(key);

    if(it==table.end())
        return std::nullopt;

    return it->second;
}


bool KVStore::remove(const std::string& key){
    std::lock_guard<std::mutex> lock(mtx);

    if(table.find(key)==table.end())
        return false;

    wal.appendDelete(key);

    table.erase(key);

    return true;
}


void KVStore::replay(){
    std::lock_guard<std::mutex> lock(mtx);

    std::ifstream in(
        "data/kv.log",
        std::ios::binary
    );

    if(!in.is_open())
        return;


    while(true){
        int k,v;

        if(!in.read((char*)&k,sizeof(k)))
            break;

        if(!in.read((char*)&v,sizeof(v)))
            break;


        std::string key(k,'\0');

        if(!in.read(&key[0],k))
            break;


        if(v==-1){
            table.erase(key);
            continue;
        }


        std::string value(v,'\0');

        if(!in.read(&value[0],v))
            break;


        table[key]=value;
    }
}