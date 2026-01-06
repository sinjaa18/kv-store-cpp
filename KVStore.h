#ifndef KVSTORE_H
#define KVSTORE_H

#include <string>
#include <unordered_map>
#include <optional>

class KVStore {
private:
    std::unordered_map<std::string, std::string> table;

public:
    KVStore() {
        load();
    }
    ~KVStore(){
        flush();
    }

    bool put(const std::string& key, const std::string& value);
    std::optional<std::string> get(const std::string& key) const;
    bool remove(const std::string& key);
    void flush();
    void load();
};

#endif
