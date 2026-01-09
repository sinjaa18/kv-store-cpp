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
        //load();
        replay();
    }
    ~KVStore(){
        flush();
    }
    //CRUD
    bool put(const std::string& key, const std::string& value);
    std::optional<std::string> get(const std::string& key) const;
    bool remove(const std::string& key);

    //memory
    void flush();
    void load();
    
    //bin-file
    void append(const std::string &key,const std::string &value);
    void replay();
};

#endif
