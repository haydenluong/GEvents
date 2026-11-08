#include "benchmark.h"
#include "event_loader.h"
#include "hash_table.h"
#include "query.h"
#include "ranking.h"
#include "trie.h"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace {

const std::size_t rowsPerPage = 100;
const int suggestionLimit = 10;
const int benchmarkRepeats = 3;

struct SearchData {
    std::vector<Event> events;
    Trie trie;
    HashTable hash;
};

std::vector<std::string> splitFields(const std::string& line) {
    std::vector<std::string> fields;
    std::string field;
    std::istringstream stream(line);
    while (std::getline(stream, field, '\t')) {
        fields.push_back(field);
    }
    if (!line.empty() && line.back() == '\t') {
        fields.emplace_back();
    }
    return fields;
}

std::string today() {
    std::time_t now = std::time(nullptr);
    std::tm* local = std::localtime(&now);
    if (local == nullptr) {
        throw std::runtime_error("Could not read today's date");
    }
    std::ostringstream date;
    date << std::put_time(local, "%Y-%m-%d");
    return date.str();
}

bool smallerId(const Event& a, const Event& b) {
    return a.id < b.id;
}

void loadData(const std::string& path, SearchData& data) {
    data.events = readEventsFile(path);
    if (data.events.empty()) {
        throw std::runtime_error("Dataset has no events");
    }

    std::sort(data.events.begin(), data.events.end(), smallerId);
    for (std::size_t i = 1; i < data.events.size(); ++i) {
        if (data.events[i - 1].id == data.events[i].id) {
            throw std::runtime_error("Duplicate event ID: " + std::to_string(data.events[i].id));
        }
    }

    for (const Event& event : data.events) {
        for (const std::string& keyword : event.keywords) {
            data.trie.insert(keyword, event.id);
            data.hash.insert(keyword, event.id);
        }
    }
}

std::vector<Event> eventsWithIds(const std::vector<Event>& events, const std::vector<int>& ids) {
    std::vector<Event> found;
    for (int id : ids) {
        Event target;
        target.id = id;
        auto spot = std::lower_bound(events.begin(), events.end(), target, smallerId);
        if (spot != events.end() && spot->id == id) {
            found.push_back(*spot);
        }
    }
    return found;
}

std::string joinWords(const std::vector<std::string>& words) {
    std::string joined;
    for (const std::string& word : words) {
        if (!joined.empty()) {
            joined += ' ';
        }
        joined += word;
    }
    return joined;
}

void sendEvent(const RankedEvent& item) {
    const Event& event = item.event;
    std::cout << "event\t" << event.id << '\t' << event.title << '\t' << event.date << '\t'
              << event.category << '\t' << event.location << '\t'
              << std::fixed << std::setprecision(3) << item.score << std::defaultfloat << '\t'
              << event.description << '\t' << joinWords(event.keywords) << '\n';
}

void search(const SearchData& data, const std::vector<std::string>& fields) {
    if (fields.size() != 8) {
        throw std::invalid_argument("Search needs 8 fields");
    }

    std::vector<std::string> keywords = parseQuery(fields[2]);
    SearchOptions options;
    options.categories = parseQuery(fields[3]);
    options.interests = parseQuery(fields[4]);
    options.startDate = fields[5];
    options.endDate = fields[6];
    options.referenceDate = today();
    std::size_t first = std::stoul(fields[7]);

    std::vector<RankedEvent> results;
    double lookupMicroseconds = 0;
    if (keywords.empty()) {
        results = rankEvents(data.events, keywords, options);
    } else {
        std::vector<int> ids;
        auto start = std::chrono::steady_clock::now();
        if (fields[1] == "trie") {
            ids = findMatchingEvents(data.trie, keywords);
        } else {
            ids = findMatchingEvents(data.hash, keywords);
        }
        auto stop = std::chrono::steady_clock::now();
        lookupMicroseconds = std::chrono::duration<double, std::micro>(stop - start).count();
        results = rankEvents(eventsWithIds(data.events, ids), keywords, options);
    }

    std::cout << "found\t" << results.size() << '\t' << lookupMicroseconds << '\n';
    for (std::size_t i = first; i < results.size() && i < first + rowsPerPage; ++i) {
        sendEvent(results[i]);
    }
}

void suggest(const SearchData& data, const std::vector<std::string>& fields) {
    std::string prefix;
    if (fields.size() > 1) {
        std::vector<std::string> words = parseQuery(fields[1]);
        if (!words.empty()) {
            prefix = words.back();
        }
    }

    for (const std::string& word : data.trie.autocomplete(prefix, suggestionLimit)) {
        std::cout << "word\t" << word << '\n';
    }
}

void benchmark(const SearchData& data) {
    if (data.events.size() < 10000) {
        throw std::invalid_argument("Load at least 10,000 events to run benchmarks");
    }

    std::vector<BenchmarkResult> results =
        runBenchmark(data.events, {10000, 25000, 50000, 100000}, benchmarkRepeats);
    for (const BenchmarkResult& result : results) {
        std::cout << "result\t" << result.structure << '\t' << result.events << '\t'
                  << result.queries << '\t' << result.insertMs << '\t' << result.searchUs << '\t'
                  << result.memoryBytes << '\t' << result.matches << '\n';
    }
}

void answer(const SearchData& data, const std::string& line) {
    std::vector<std::string> fields = splitFields(line);
    if (fields.empty()) {
        throw std::invalid_argument("Empty request");
    }

    if (fields[0] == "search") {
        search(data, fields);
    } else if (fields[0] == "suggest") {
        suggest(data, fields);
    } else if (fields[0] == "benchmark") {
        benchmark(data);
    } else {
        throw std::invalid_argument("Unknown request: " + fields[0]);
    }
}

}

int main(int argc, char* argv[]) {
    std::string path = "data/events.tsv";
    if (argc > 1) {
        path = argv[1];
    }

    SearchData data;
    try {
        loadData(path, data);
    } catch (const std::exception& error) {
        std::cout << "error\t" << error.what() << std::endl;
        return 1;
    }
    std::cout << "ready\t" << data.events.size() << std::endl;

    std::string line;
    while (std::getline(std::cin, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        try {
            answer(data, line);
        } catch (const std::exception& error) {
            std::cout << "error\t" << error.what() << '\n';
        }
        // the window waits for this line to know the reply is complete
        std::cout << "done" << std::endl;
    }
}
