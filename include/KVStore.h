#ifndef KVSTORE_H
#define KVSTORE_H

#include "MemTable.h"
#include "WAL.h"
#include "BlockCache.h"
#include "SSTableReader.h"
#include "SSTableWriter.h"
#include <mutex>
#include <memory>
#include <vector>
#include <atomic>

class KVStore {
    MemTable memtable;
    WAL wal;
    std::mutex mtx;
    
    std::vector<std::shared_ptr<SSTableReader>> sstables;
    uint32_t nextSSTableId = 1;
    
    std::atomic<uint64_t> sequenceNumber{0};
    std::shared_ptr<BlockCache> blockCache;

    void flushMemTable();
    void replay();
    void compact();
    void loadSSTables();

public:
    KVStore();
    
    bool put(const std::string& key, const std::string& value);
    std::optional<std::string> get(const std::string& key);
    bool remove(const std::string& key);
    std::shared_ptr<BlockCache> getBlockCache() { return blockCache; }
};

#endif