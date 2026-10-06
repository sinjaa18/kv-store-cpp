#ifndef SKIPLIST_H
#define SKIPLIST_H

#include<vector>
#include<utility>
#include<string>
#include<optional>

class SkipList {
    struct Node {
        std::string key;
        std::string value;
        std::vector<Node*> next;
        Node(const std::string& k,const std::string& v,int level
            ):key(k), value(v), next(level, nullptr) {
        }
    };
    static constexpr int MAX_LEVEL = 16;
    int currentLevel;
    Node* head;
public:
    SkipList();
    ~SkipList();
    bool put(const std::string& key,const std::string& value);
    std::optional<std::string> get(const std::string& key)const;
    bool remove(const std::string& key);
    std::vector<std::pair<std::string,std::string>> entries()const;
private:
    int randomLevel();
};

#endif