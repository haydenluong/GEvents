#include "hash_table.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <stdexcept>

HashTable::HashTable(std::size_t initialBuckets)
    : buckets(std::max<std::size_t>(initialBuckets, 1)) {}

HashTable::~HashTable() {
    for (auto& head : buckets) {
        while (head) {
            auto node = std::move(head);
            head = std::move(node->next);
        }
    }
}

std::size_t HashTable::hash(const std::string& keyword) {
    std::uint64_t value = 14695981039346656037ULL;
    for (unsigned char ch : keyword) {
        value ^= ch;
        value *= 1099511628211ULL;
    }
    return static_cast<std::size_t>(value);
}

void HashTable::resize() {
    if (buckets.size() > std::numeric_limits<std::size_t>::max() / 2) {
        throw std::length_error("Hash table capacity exceeded");
    }
    std::vector<std::unique_ptr<Node>> larger(buckets.size() * 2);
    for (auto& head : buckets) {
        while (head) {
            auto node = std::move(head);
            head = std::move(node->next);
            const auto slot = hash(node->keyword) % larger.size();
            node->next = std::move(larger[slot]);
            larger[slot] = std::move(node);
        }
    }
    buckets.swap(larger);
}

void HashTable::insert(const std::string& keyword, int eventId) {
    if (keyword.empty()) {
        return;
    }
    auto slot = hash(keyword) % buckets.size();
    for (auto* node = buckets[slot].get(); node; node = node->next.get()) {
        if (node->keyword == keyword) {
            if (eventId > node->ids.back()) {
                node->ids.push_back(eventId);
                return;
            }
            if (eventId == node->ids.back()) {
                return;
            }
            auto position = std::lower_bound(node->ids.begin(), node->ids.end(), eventId);
            if (position == node->ids.end() || *position != eventId) {
                node->ids.insert(position, eventId);
            }
            return;
        }
    }
    auto node = std::make_unique<Node>(keyword);
    node->ids.push_back(eventId);
    if (static_cast<double>(keywords + 1) / buckets.size() > 0.75) {
        resize();
        slot = hash(keyword) % buckets.size();
    }
    node->next = std::move(buckets[slot]);
    buckets[slot] = std::move(node);
    ++keywords;
}

std::vector<int> HashTable::find(const std::string& keyword) const {
    const auto slot = hash(keyword) % buckets.size();
    for (auto* node = buckets[slot].get(); node; node = node->next.get()) {
        if (node->keyword == keyword) {
            return node->ids;
        }
    }
    return {};
}

std::size_t HashTable::memoryBytes() const {
    std::size_t bytes = buckets.capacity() * sizeof(decltype(buckets)::value_type);
    for (const auto& head : buckets) {
        for (auto* node = head.get(); node; node = node->next.get()) {
            bytes += sizeof(Node) + node->ids.capacity() * sizeof(int);
            // Short strings store characters inside the node itself.
            const auto characters = reinterpret_cast<std::uintptr_t>(node->keyword.data());
            const auto object = reinterpret_cast<std::uintptr_t>(&node->keyword);
            if (characters < object || characters - object >= sizeof(std::string)) {
                bytes += node->keyword.capacity() + 1;
            }
        }
    }
    return bytes;
}

std::size_t HashTable::bucketCount() const { return buckets.size(); }
std::size_t HashTable::keywordCount() const { return keywords; }
