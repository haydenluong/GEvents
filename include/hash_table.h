#pragma once

#include "index.h"

#include <memory>

class HashTable : public Index {
public:
    explicit HashTable(std::size_t initialBuckets = 16);
    ~HashTable() override;
    HashTable(const HashTable&) = delete;
    HashTable& operator=(const HashTable&) = delete;

    void insert(const std::string& keyword, int eventId) override;
    std::vector<int> find(const std::string& keyword) const override;
    std::size_t memoryBytes() const override;
    std::size_t bucketCount() const;
    std::size_t keywordCount() const;

private:
    struct Node {
        std::string keyword;
        std::vector<int> ids;
        std::unique_ptr<Node> next;
        explicit Node(const std::string& word) : keyword(word) {}
    };

    std::vector<std::unique_ptr<Node>> buckets;
    std::size_t keywords = 0;

    static std::size_t hash(const std::string& keyword);
    void resize();
};
