#include "SkipList.h"
#include <cstdlib>
#include <vector>
#include <ctime>

SkipList::SkipList() :currentLevel(1) {
    srand(static_cast<unsigned int>(time(0)));
    head = new Node("", std::nullopt, 0, MAX_LEVEL);
}

bool SkipList::put(uint64_t seq, const std::string& key, const std::optional<std::string>& value) {
    std::vector<Node*> update(MAX_LEVEL, nullptr);
    Node* cur = head;

    for (int lvl = currentLevel - 1;lvl >= 0;lvl--) {
        while (cur->next[lvl] && cur->next[lvl]->key < key)
            cur = cur->next[lvl];
        update[lvl] = cur;
    }

    cur = cur->next[0];

    if (cur && cur->key == key) {
        cur->value = value;
        cur->seq = seq;
        return true;
    }

    int level = randomLevel();

    if (level > currentLevel) {
        for (int i = currentLevel;i < level;i++)
            update[i] = head;
        currentLevel = level;
    }

    Node* node = new Node(key, value, seq, level);

    for (int i = 0;i < level;i++) {
        node->next[i] = update[i]->next[i];
        update[i]->next[i] = node;
    }
    numEntries++;

    return true;
}


std::optional<KVPair> SkipList::get(const std::string& key)const {
    Node* cur = head;
    for (int lvl = currentLevel - 1;lvl >= 0;lvl--) {
        while (cur->next[lvl] && cur->next[lvl]->key < key)
            cur = cur->next[lvl];
    }

    cur=cur->next[0];
    if (cur && cur->key == key) 
        return KVPair{cur->seq, cur->key, cur->value};
    return std::nullopt;
}


bool SkipList::remove(uint64_t seq, const std::string& key) {
    std::vector<Node*> update(MAX_LEVEL, nullptr);
    Node* cur = head;
    for (int lvl = currentLevel - 1;lvl >= 0;lvl--) {
        while (cur->next[lvl] &&
            cur->next[lvl]->key < key)
            cur = cur->next[lvl];
        update[lvl] = cur;
    }
    cur = cur->next[0];

    if (!cur || cur->key != key)
        return false;
    
    for (int i = 0;i < currentLevel;i++) {
        if (update[i]->next[i] != cur)
            break;
        update[i]->next[i] = cur->next[i];
    }
    delete cur;
    while (currentLevel > 1 &&
        head->next[currentLevel - 1] == nullptr)
        currentLevel--;
    numEntries--;
    return true;
}

int SkipList::randomLevel() {
    int level = 1;
    while (level < MAX_LEVEL && rand() % 2)
        level++;
    return level;
}

std::vector<KVPair> SkipList::entries()const{
    std::vector < KVPair >result;
    Node* cur=head->next[0];

    while(cur){
        result.push_back({cur->seq, cur->key,cur->value});
        cur=cur->next[0];
    }
    return result;
}

SkipList::~SkipList() {
    Node* cur = head;
    while (cur) {
        Node* nxt = cur->next[0];
        delete cur;
        cur = nxt;
    }
}

size_t SkipList::size() const {
    return numEntries;
}

void SkipList::clear() {
    Node* cur = head->next[0];
    while (cur) {
        Node* nxt = cur->next[0];
        delete cur;
        cur = nxt;
    }
    for (int i = 0; i < MAX_LEVEL; ++i) {
        head->next[i] = nullptr;
    }
    currentLevel = 1;
    numEntries = 0;
}