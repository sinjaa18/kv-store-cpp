#ifndef KVSTORE_H
#define KVSTORE_H

#include<string>
#include<unordered_map>
#include<optional>
#include<mutex>
#include "WAL.h"

class KVStore{

    std::unordered_map
    <std::string,std::string> table;

    mutable std::mutex mtx;

    WAL wal;

public:

    KVStore();

    bool put(
    const std::string& key,
    const std::string& value);

    std::optional<std::string>
    get(const std::string& key)const;

    bool remove(
    const std::string& key);

    void replay();
};

#endif