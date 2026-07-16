#include "SkipList.h"
#include <cstdlib>
#include <vector>

SkipList::SkipList() :currentLevel(1) {
    head = new Node("", "", MAX_LEVEL);
}

bool SkipList::put(const std::string& key, const std::string& value) {
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
        return true;
    }

    int level = randomLevel();

    if (level > currentLevel) {
        for (int i = currentLevel;i < level;i++)
            update[i] = head;
        currentLevel = level;
    }

    Node* node = new Node(key, value, level);

    for (int i = 0;i < level;i++) {
        node->next[i] = update[i]->next[i];
        update[i]->next[i] = node;
    }

    return true;
}


std::optional<std::string> SkipList::get(const std::string& key)const {
    Node* cur = head;
    for (int lvl = currentLevel - 1;lvl >= 0;lvl--) {
        while (cur->next[lvl] && cur->next[lvl]->key < key)
            cur = cur->next[lvl];
    }

    cur=cur->next[0];
    if (cur && cur->key == key) 
        return cur->value;
    return std::nullopt;
}


bool SkipList::remove(const std::string& key) {
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
    return true;
}

int SkipList::randomLevel() {
    int level = 1;
    while (level < MAX_LEVEL && rand() % 2)
        level++;
    return level;
}

SkipList::~SkipList() {
    Node* cur = head;
    while (cur) {
        Node* nxt = cur->next[0];
        delete cur;
        cur = nxt;
    }
}