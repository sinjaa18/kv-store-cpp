#ifndef SKIPLIST_H
#define SKIPLIST_H

#include<vector>
#include<utility>
#include<string>
#include<optional>
#include "KVPair.h"

class SkipList {
    struct Node {
        std::string key;
        std::optional<std::string> value;
        uint64_t seq;
        std::vector<Node*> next;
        Node(const std::string& k,const std::optional<std::string>& v, uint64_t s, int level
            ):key(k), value(v), seq(s), next(level, nullptr) {
        }
    };
    static constexpr int MAX_LEVEL = 16;
    int currentLevel;
    Node* head;
    size_t numEntries = 0;
public:
    SkipList();
    ~SkipList();
    bool put(uint64_t seq, const std::string& key,const std::optional<std::string>& value);
    std::optional<KVPair> get(const std::string& key)const;
    bool remove(uint64_t seq, const std::string& key);
    std::vector<KVPair> entries()const;
    size_t size() const;
    void clear();
private:
    int randomLevel();
};

#endif