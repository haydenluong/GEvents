#include "benchmark.h"

#include "hash_table.h"
#include "trie.h"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <stdexcept>

namespace {

using Clock = std::chrono::steady_clock;

const int searchPassesPerRun = 200;

double millisecondsSince(Clock::time_point start) {
    return std::chrono::duration<double, std::milli>(Clock::now() - start).count();
}

double microsecondsSince(Clock::time_point start) {
    return std::chrono::duration<double, std::micro>(Clock::now() - start).count();
}

void insertEvents(Index& index, const std::vector<Event>& events, std::size_t count) {
    for (std::size_t i = 0; i < count; ++i) {
        for (const std::string& keyword : events[i].keywords) {
            index.insert(keyword, events[i].id);
        }
    }
}

std::vector<std::string> queryWords(const std::vector<Event>& events, std::size_t count) {
    std::vector<std::string> words;
    for (std::size_t i = 0; i < count; ++i) {
        words.insert(words.end(), events[i].keywords.begin(), events[i].keywords.end());
    }
    std::sort(words.begin(), words.end());
    words.erase(std::unique(words.begin(), words.end()), words.end());
    for (const char* miss : {"zzz", "unknownword", "cod", "workshops"}) {
        words.push_back(miss);
    }
    return words;
}

template <typename Structure>
double timeInsertion(const std::vector<Event>& events, std::size_t count, int repeats,
                     std::size_t& memory) {
    double total = 0;
    for (int run = 0; run <= repeats; ++run) {
        Structure index;
        auto start = Clock::now();
        insertEvents(index, events, count);
        double elapsed = millisecondsSince(start);
        // the first run warms the cache and allocator, so it is left out of the average
        if (run > 0) {
            total += elapsed;
        }
        memory = index.memoryBytes();
    }
    return total / repeats;
}

template <typename Structure>
double timeSearch(const std::vector<Event>& events, std::size_t count,
                  const std::vector<std::string>& queries, int repeats, std::size_t& matches) {
    Structure index;
    insertEvents(index, events, count);

    double total = 0;
    for (int run = 0; run <= repeats; ++run) {
        std::size_t found = 0;
        auto start = Clock::now();
        for (int pass = 0; pass < searchPassesPerRun; ++pass) {
            for (const std::string& query : queries) {
                found += index.find(query).size();
            }
        }
        double elapsed = microsecondsSince(start);
        if (run > 0) {
            total += elapsed;
        }
        matches = found / searchPassesPerRun;
    }
    return total / (static_cast<double>(repeats) * searchPassesPerRun * queries.size());
}

template <typename Structure>
BenchmarkResult measure(const std::string& name, const std::vector<Event>& events,
                        std::size_t count, const std::vector<std::string>& queries, int repeats) {
    BenchmarkResult result;
    result.structure = name;
    result.events = count;
    result.queries = queries.size();
    result.insertMs = timeInsertion<Structure>(events, count, repeats, result.memoryBytes);
    result.searchUs = timeSearch<Structure>(events, count, queries, repeats, result.matches);
    return result;
}

}

std::vector<BenchmarkResult> runBenchmark(const std::vector<Event>& events,
                                          const std::vector<std::size_t>& sizes, int repeats) {
    if (repeats < 1) {
        throw std::invalid_argument("repeats must be at least 1");
    }

    std::vector<BenchmarkResult> results;
    for (std::size_t size : sizes) {
        if (size > events.size()) {
            continue;
        }
        std::vector<std::string> queries = queryWords(events, size);
        results.push_back(measure<Trie>("Trie", events, size, queries, repeats));
        results.push_back(measure<HashTable>("HashTable", events, size, queries, repeats));
    }
    return results;
}

void printResults(const std::vector<BenchmarkResult>& results, std::ostream& out) {
    out << std::left << std::setw(11) << "Structure" << std::right << std::setw(9) << "Events"
        << std::setw(9) << "Queries" << std::setw(14) << "Insert (ms)" << std::setw(16)
        << "Search (us/q)" << std::setw(14) << "Memory (KB)" << std::setw(12) << "Matches" << '\n';
    out << std::fixed;
    for (const BenchmarkResult& result : results) {
        out << std::left << std::setw(11) << result.structure << std::right << std::setw(9)
            << result.events << std::setw(9) << result.queries << std::setw(14)
            << std::setprecision(2) << result.insertMs << std::setw(16) << std::setprecision(3)
            << result.searchUs << std::setw(14) << std::setprecision(1)
            << result.memoryBytes / 1024.0 << std::setw(12) << result.matches << '\n';
    }
}

void writeResultsCsv(const std::vector<BenchmarkResult>& results, const std::string& path) {
    std::ofstream file(path);
    if (!file) {
        throw std::runtime_error("Could not write " + path);
    }
    file << "structure,events,queries,insert_ms,search_us,memory_bytes,matches\n";
    for (const BenchmarkResult& result : results) {
        file << result.structure << ',' << result.events << ',' << result.queries << ','
             << result.insertMs << ',' << result.searchUs << ',' << result.memoryBytes << ','
             << result.matches << '\n';
    }
}
