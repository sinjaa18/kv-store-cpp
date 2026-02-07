#ifndef KVSTORE_H
#define KVSTORE_H

#include <string>
#include <unordered_map>
#include <optional>
#include <mutex>

class KVStore {
private:
    std::unordered_map<std::string, std::string> table;
    mutable std::mutex mtx;   // Added mutex

public:
    KVStore() {
        replay();
    }

    ~KVStore() = default;

    // CRUD
    bool put(const std::string& key, const std::string& value);
    std::optional<std::string> get(const std::string& key) const;
    bool remove(const std::string& key);

    //Old CSV—unused
    void flush();
    void load();

    // Binary log
    void append_put(const std::string &key,const std::string &value);
    void append_remove(const std::string &key);
    void replay();
};

#endif
