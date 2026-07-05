#ifndef MEMTABLE_H
#define MEMTABLE_H

#include<string>
#include<unordered_map>
#include<optional>

class MemTable {
    std::unordered_map<std::string, std::string> data;
public:
    bool put(const std::string& key,const std::string& value);
    std::optional<std::string> get(const std::string& key)const;
    bool remove(const std::string& key);
};

#endif