#include "event.h"
#include "hash_table.h"
#include "trie.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

std::vector<Event> sampleEvents() {
    return {
        {0, "Hack Night", "2026-11-06", "CSE", "technology", "", {"hackathon", "coding", "pizza"}},
        {1, "Startup Pitch", "2026-11-07", "Innovation Hub", "entrepreneurship", "", {"startup", "pitch", "coding"}},
        {2, "Pizza Social", "2026-11-08", "Reitz", "social", "", {"pizza", "games", "pizza"}},
        {3, "Code Review Club", "2026-11-09", "Marston", "academic", "", {"code", "coding", "review", "coffee"}},
        {4, "Hack Workshop", "2026-11-10", "CSE", "technology", "", {"hack", "hackathon", "hacking"}},
    };
}

void loadEvents(Index& index, const std::vector<Event>& events) {
    for (const auto& event : events) {
        for (const auto& keyword : event.keywords) index.insert(keyword, event.id);
    }
}

void testInsertAndFind() {
    Trie trie;
    loadEvents(trie, sampleEvents());
    require(trie.find("coding") == std::vector<int>({0, 1, 3}), "Shared keyword across events");
    require(trie.find("pizza") == std::vector<int>({0, 2}), "Keyword repeated in one event");
    require(trie.find("hack") == std::vector<int>{4}, "Keyword that is a prefix of another");
    require(trie.find("hackathon") == std::vector<int>({0, 4}), "Keyword that extends another");
    require(trie.find("code") == std::vector<int>{3}, "Keyword on a shared branch");
    require(trie.find("hac").empty(), "Prefix only matched");
    require(trie.find("hackathons").empty(), "Longer word matched");
    require(trie.find("music").empty(), "Unknown keyword matched");
    require(trie.find("").empty(), "Empty keyword matched");
    trie.insert("", 5);
    require(trie.find("").empty(), "Empty keyword was stored");
    auto copy = trie.find("coding");
    copy.clear();
    require(trie.find("coding").size() == 3, "Find exposed stored posting");
    const Index& index = trie;
    require(index.find("startup") == std::vector<int>{1}, "Find through Index");
}

void testMatchesHashTable() {
    auto events = sampleEvents();
    Trie trie;
    HashTable table;
    loadEvents(trie, events);
    loadEvents(table, events);
    for (const auto& event : events) {
        for (const auto& keyword : event.keywords) {
            require(trie.find(keyword) == table.find(keyword), "Trie and hash table disagree");
        }
    }
    for (const auto& word : {"hac", "music", "codingx", "p"}) {
        require(trie.find(word) == table.find(word), "Trie and hash table disagree on a miss");
    }
}

void testMemoryAndScaling() {
    Trie trie;
    const auto emptyMemory = trie.memoryBytes();
    require(emptyMemory > 0, "Root node not counted");
    trie.insert("code", 0);
    const auto oneKeyword = trie.memoryBytes();
    require(oneKeyword > emptyMemory, "Memory did not grow");
    trie.insert("code", 1);
    require(trie.memoryBytes() >= oneKeyword, "Memory shrank");
    for (int id = 0; id < 100000; ++id) {
        trie.insert("keyword" + std::to_string(id % 500), id);
        trie.insert("shared", id);
    }
    auto shared = trie.find("shared");
    require(shared.size() == 100000, "Lost postings at scale");
    for (std::size_t i = 1; i < shared.size(); ++i) {
        require(shared[i - 1] < shared[i], "Posting list not sorted");
    }
    require(trie.find("keyword7").size() == 200, "Wrong posting count at scale");
    require(trie.memoryBytes() >= 100000 * sizeof(int), "Posting memory missing");
}

void testAutocomplete() {
    using Words = std::vector<std::string>;
    Trie trie;
    loadEvents(trie, sampleEvents());
    require(trie.autocomplete("hack", 10) == Words({"hack", "hackathon", "hacking"}), "Prefix that is a keyword");
    require(trie.autocomplete("co", 10) == Words({"code", "coding", "coffee"}), "Alphabetical order");
    require(trie.autocomplete("co", 2) == Words({"code", "coding"}), "Limit not respected");
    require(trie.autocomplete("hack", 1) == Words{"hack"}, "Limit of one");
    require(trie.autocomplete("pizza", 10) == Words{"pizza"}, "Full keyword as prefix");
    require(trie.autocomplete("pizzas", 10).empty(), "Longer prefix matched");
    require(trie.autocomplete("x", 10).empty(), "Unknown prefix matched");
    require(trie.autocomplete("hack", 0).empty(), "Zero limit returned words");
    require(trie.autocomplete("hack", -3).empty(), "Negative limit returned words");
    require(trie.autocomplete("", 3) == Words({"code", "coding", "coffee"}), "Empty prefix start");
    require(trie.autocomplete("", 100).size() == 11, "Empty prefix should list every keyword");
    require(Trie().autocomplete("", 5).empty(), "Empty trie returned words");
}

int main() {
    try {
        testInsertAndFind();
        testMatchesHashTable();
        testMemoryAndScaling();
        testAutocomplete();
        std::cout << "Trie tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
