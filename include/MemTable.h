#ifndef MEMTABLE_H
#define MEMTABLE_H

#include<string>
#include "SkipList.h"
#include<optional>
#include "KVPair.h"

class MemTable {
    SkipList data;
public:
    bool put(uint64_t seq, const std::string& key,const std::optional<std::string>& value);
    std::optional<KVPair> get(const std::string& key)const;
    bool remove(uint64_t seq, const std::string& key);
    std::vector<KVPair> entries() const;
    size_t size() const;
    void clear();
};

#endif