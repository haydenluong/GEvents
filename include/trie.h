#pragma once

#include "index.h"

#include <cstddef>
#include <string>
#include <vector>

class Trie : public Index {
public:
    Trie();

    void insert(const std::string& keyword, int eventId) override;
    std::vector<int> find(const std::string& keyword) const override;
    std::size_t memoryBytes() const override;

private:
    struct Child {
        char letter;
        int node;
    };

    struct Node {
        std::vector<Child> children;
        std::vector<int> eventIds;
    };

    std::vector<Node> nodes;

    int childOf(int node, char letter) const;
    int addChild(int node, char letter);
    int nodeFor(const std::string& keyword) const;
};
