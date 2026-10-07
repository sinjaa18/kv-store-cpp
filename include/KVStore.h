#ifndef KVSTORE_H
#define KVSTORE_H

#include<string>
#include<optional>
#include<mutex>
#include "WAL.h"
#include "MemTable.h"
#include "SSTableReader.h"
#include <memory>
#include <vector>

class KVStore {
    MemTable memtable;
    WAL wal;
    mutable std::mutex mtx;
    std::vector<std::shared_ptr<SSTableReader>> sstables;
    int nextSSTableId = 1;

    void flushMemTable();
    void compact();
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