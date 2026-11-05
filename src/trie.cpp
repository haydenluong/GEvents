#include "trie.h"

#include <algorithm>

namespace {

template <typename Children>
auto findSlot(Children& children, char letter) {
    return std::lower_bound(children.begin(), children.end(), letter,
                            [](const auto& child, char target) { return child.letter < target; });
}

}

Trie::Trie() : nodes(1) {}

void Trie::insert(const std::string& keyword, int eventId) {
    if (keyword.empty()) {
        return;
    }

    int current = 0;
    for (char letter : keyword) {
        int next = childOf(current, letter);
        if (next < 0) {
            next = addChild(current, letter);
        }
        current = next;
    }

    std::vector<int>& ids = nodes[current].eventIds;
    if (ids.empty() || ids.back() != eventId) {
        ids.push_back(eventId);
    }
}

std::vector<int> Trie::find(const std::string& keyword) const {
    int node = nodeFor(keyword);
    if (node < 0) {
        return {};
    }
    return nodes[node].eventIds;
}

std::size_t Trie::memoryBytes() const {
    std::size_t total = nodes.capacity() * sizeof(Node);
    for (const Node& node : nodes) {
        total += node.children.capacity() * sizeof(Child);
        total += node.eventIds.capacity() * sizeof(int);
    }
    return total;
}

std::vector<std::string> Trie::autocomplete(const std::string& prefix, int limit) const {
    std::vector<std::string> matches;
    if (limit <= 0) {
        return matches;
    }

    int start = prefix.empty() ? 0 : nodeFor(prefix);
    if (start < 0) {
        return matches;
    }

    std::string word = prefix;
    collectWords(start, word, static_cast<std::size_t>(limit), matches);
    return matches;
}

void Trie::collectWords(int node, std::string& word, std::size_t limit,
                        std::vector<std::string>& matches) const {
    if (!nodes[node].eventIds.empty()) {
        matches.push_back(word);
    }

    for (const Child& child : nodes[node].children) {
        if (matches.size() >= limit) {
            return;
        }
        word.push_back(child.letter);
        collectWords(child.node, word, limit, matches);
        word.pop_back();
    }
}

int Trie::childOf(int node, char letter) const {
    const std::vector<Child>& children = nodes[node].children;
    auto slot = findSlot(children, letter);
    if (slot == children.end() || slot->letter != letter) {
        return -1;
    }
    return slot->node;
}

int Trie::addChild(int node, char letter) {
    int created = static_cast<int>(nodes.size());
    nodes.emplace_back();

    std::vector<Child>& children = nodes[node].children;
    children.insert(findSlot(children, letter), Child{letter, created});
    return created;
}

int Trie::nodeFor(const std::string& keyword) const {
    if (keyword.empty()) {
        return -1;
    }

    int current = 0;
    for (char letter : keyword) {
        current = childOf(current, letter);
        if (current < 0) {
            return -1;
        }
    }
    return current;
}
