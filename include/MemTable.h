#ifndef MEMTABLE_H
#define MEMTABLE_H

#include<string>
#include "SkipList.h"
#include<optional>

class MemTable {
    SkipList data;
public:
    bool put(const std::string& key, const std::string& value);
    std::optional<std::string> get(const std::string& key) const;
    bool remove(const std::string& key);
    std::vector<std::pair<std::string, std::string>> entries() const;
    size_t size() const;
    void clear();
};

#endif