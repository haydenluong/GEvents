#include "hash_table.h"
#include "ranking.h"

#include <cmath>
#include <iostream>
#include <stdexcept>

void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

template <typename Function>
void rejects(Function operation) {
    bool threw = false;
    try { operation(); }
    catch (const std::invalid_argument&) { threw = true; }
    require(threw, "Expected invalid_argument");
}

void testHashTable() {
    HashTable table(4);
    const auto emptyMemory = table.memoryBytes();
    require(table.find("missing").empty(), "Unknown keyword");
    // These keys collide in the initial four buckets with FNV-1a.
    table.insert("a", 8);
    table.insert("e", 3);
    table.insert("i", 7);
    require(table.bucketCount() == 4, "Resize happened too early");
    require(table.find("a") == std::vector<int>{8}, "Collision lost a");
    require(table.find("e") == std::vector<int>{3}, "Collision lost e");
    require(table.find("i") == std::vector<int>{7}, "Collision lost i");
    table.insert("a", 2);
    table.insert("a", 8);
    table.insert("a", 5);
    require(table.find("a") == std::vector<int>({2, 5, 8}), "Posting order or duplicates");
    require(table.keywordCount() == 3, "Duplicate keyword counted");
    table.insert("o", 4);
    require(table.bucketCount() == 8, "Expected load-factor resize");
    require(table.find("e") == std::vector<int>{3}, "Rehash lost a collision");
    for (int i = 0; i < 10000; ++i) table.insert("keyword" + std::to_string(i), i);
    for (int i = 0; i < 10000; ++i) {
        require(table.find("keyword" + std::to_string(i)) == std::vector<int>{i}, "Scaling lost a posting");
    }
    require(table.memoryBytes() > emptyMemory, "Memory did not grow");
    const auto beforeLongKey = table.memoryBytes();
    table.insert(std::string(200, 'x'), 42);
    require(table.memoryBytes() >= beforeLongKey + 201 + sizeof(int), "Long key allocation missing");
    auto copy = table.find("a");
    copy.clear();
    require(table.find("a").size() == 3, "Find exposed stored posting");
    HashTable zero(0);
    zero.insert("", 0);
    require(zero.find("") == std::vector<int>{0}, "Zero capacity or empty key");
}

void testRanking() {
    std::vector<Event> events = {
        {3, "Later tech", "2026-11-07", "Hall", "technology", "", {"code", "learn"}},
        {2, "Today arts", "2026-11-05", "Hall", "arts", "", {"code", "learn"}},
        {1, "Tomorrow tech", "2026-11-06", "Hall", "technology", "", {"code", "learn"}},
        {4, "Partial", "2026-11-05", "Hall", "technology", "", {"code"}}
    };
    SearchOptions options;
    options.referenceDate = "2026-11-05";
    options.interests = {"technology"};
    auto ranked = rankEvents(events, {"code", "learn", "code"}, options);
    require(ranked.size() == 3, "AND search or duplicate keywords");
    require(ranked[0].event.id == 1 && ranked[1].event.id == 3 && ranked[2].event.id == 2,
            "Interest and date ordering");
    require(std::abs(ranked[0].score - 8.5) < 1e-9, "Incorrect score");
    options.startDate = "2026-11-06";
    options.endDate = "2026-11-07";
    options.categories = {"technology", "sports"};
    require(filterEvents(events, options).size() == 2, "Inclusive date or category filter");
    options.categories = {"sports"};
    require(filterEvents(events, options).empty(), "Category mismatch");
    options = {};
    require(rankEvents(events, {}, options).size() == 4, "Empty query should browse events");
    require(rankEvents(events, {"unknown"}, options).empty(), "Unknown query matched");
    std::vector<Event> ties = {events[1], events[1]};
    ties[0].id = 9;
    require(rankEvents(ties, {}, options)[0].event.id == 2, "ID tie break");
    options.startDate = "2026-11-07";
    options.endDate = "2026-11-05";
    rejects([&] { filterEvents(events, options); });
    for (const auto& date : {"2026-02-29", "1900-02-29", "2026-13-01", "2026-00-01", "2026-11-31", "2026-1-05", "abcd-11-05"}) {
        options = {};
        options.referenceDate = date;
        rejects([&] { rankEvents(events, {}, options); });
    }
    events = {{1, "Leap day", "2000-02-29", "", "arts", "", {}},
              {2, "March", "2000-03-01", "", "arts", "", {}}};
    options = {};
    options.referenceDate = "2000-02-28";
    ranked = rankEvents(events, {}, options);
    require(std::abs(ranked[0].score - 0.5) < 1e-9, "Leap-day distance");
    require(std::abs(ranked[1].score - 1.0 / 3.0) < 1e-9, "Month-boundary distance");
}

int main() {
    try {
        testHashTable();
        testRanking();
        std::cout << "Hash table and ranking tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
