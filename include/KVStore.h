#ifndef KVSTORE_H
#define KVSTORE_H

#include<string>
#include<optional>
#include<mutex>
#include "WAL.h"
#include "MemTable.h"

class KVStore {
    MemTable memtable;
    WAL wal;
    mutable std::mutex mtx;
public:
    KVStore();
    bool put(
        const std::string& key,
        const std::string& value
    );
    std::optional<std::string>
        get(
            const std::string& key
        )const;
    bool remove(
        const std::string& key
    );
    void replay();
};

#endif