#include "benchmark.h"
#include "event_loader.h"
#include "hash_table.h"
#include "query.h"
#include "ranking.h"
#include "trie.h"

#include <algorithm>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>

namespace {
std::string ask(const std::string& prompt) {
    std::cout << prompt;
    std::string answer;
    if (!std::getline(std::cin, answer)) throw std::ios_base::failure("Input closed");
    const auto first = answer.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    return answer.substr(first, answer.find_last_not_of(" \t\r\n") - first + 1);
}

std::string today() {
    const auto now = std::time(nullptr);
    const auto* local = std::localtime(&now);
    if (!local) throw std::runtime_error("Could not read today's date");
    std::ostringstream date;
    date << std::put_time(local, "%Y-%m-%d");
    return date.str();
}

struct Dataset {
    std::vector<Event> events;
    Trie trie;
    HashTable hash;

    explicit Dataset(const std::string& path) : events(readEventsFile(path)) {
        if (events.empty()) throw std::runtime_error("Dataset has no events");
        std::sort(events.begin(), events.end(), [](const Event& a, const Event& b) {
            return a.id < b.id;
        });
        for (std::size_t i = 0; i < events.size(); ++i) {
            if (i > 0 && events[i - 1].id == events[i].id) {
                throw std::runtime_error("Duplicate event ID: " + std::to_string(events[i].id));
            }
        }
        filterEvents(events, {});
        for (const auto& event : events) {
            for (const auto& keyword : event.keywords) {
                trie.insert(keyword, event.id);
                hash.insert(keyword, event.id);
            }
        }
    }
};

void showResults(const std::vector<RankedEvent>& results) {
    std::cout << results.size() << " matching events\n";
    for (std::size_t i = 0; i < results.size(); ++i) {
        if (i > 0 && i % 10 == 0 && ask("Show more? (y/N): ") != "y") break;
        const auto& item = results[i];
        const auto& event = item.event;
        std::cout << '\n' << event.id << " | " << event.title << " | score "
                  << std::fixed << std::setprecision(3) << item.score << '\n'
                  << event.date << " | " << event.category << " | " << event.location << '\n'
                  << event.description << "\nKeywords:";
        for (const auto& keyword : event.keywords) std::cout << ' ' << keyword;
        std::cout << '\n';
    }
    std::cout << std::defaultfloat << std::setprecision(6);
}

void search(const Dataset& data, bool useTrie) {
    const auto keywords = parseQuery(ask("Keywords (space-separated; blank browses all): "));
    std::cout << "Categories: social technology entrepreneurship academic sports arts career volunteering\n";
    SearchOptions options;
    options.categories = parseQuery(ask("Allowed categories (space-separated; blank allows all): "));
    options.interests = parseQuery(ask("Interest categories (space-separated; blank for none): "));
    const std::vector<std::string> categories = {"social", "technology", "entrepreneurship",
        "academic", "sports", "arts", "career", "volunteering"};
    for (const auto* choices : {&options.categories, &options.interests}) {
        for (const auto& category : *choices) {
            if (std::find(categories.begin(), categories.end(), category) == categories.end()) {
                throw std::invalid_argument("Unknown category: " + category);
            }
        }
    }
    options.startDate = ask("Start date (YYYY-MM-DD; blank for no bound): ");
    options.endDate = ask("End date (YYYY-MM-DD; blank for no bound): ");
    options.referenceDate = today();
    std::cout << "Ranking relative to " << options.referenceDate << '\n';
    filterEvents({}, options);
    if (keywords.empty()) {
        showResults(rankEvents(data.events, keywords, options));
        return;
    }
    const Index& index = useTrie ? static_cast<const Index&>(data.trie)
                                 : static_cast<const Index&>(data.hash);
    const auto ids = findMatchingEvents(index, keywords);
    std::vector<Event> candidates;
    for (int id : ids) {
        const auto event = std::lower_bound(data.events.begin(), data.events.end(), id,
            [](const Event& value, int target) { return value.id < target; });
        if (event != data.events.end() && event->id == id) candidates.push_back(*event);
    }
    showResults(rankEvents(candidates, keywords, options));
}

void autocomplete(const Dataset& data) {
    const auto words = parseQuery(ask("Keyword prefix (blank lists keywords): "));
    if (words.size() > 1) throw std::invalid_argument("Enter one prefix");
    const auto matches = data.trie.autocomplete(words.empty() ? "" : words.front(), 10);
    if (matches.empty()) std::cout << "No matching keywords\n";
    for (const auto& word : matches) std::cout << word << '\n';
}

void benchmark(const Dataset& data) {
    if (data.events.size() < 10000) {
        std::cout << "Load at least 10,000 events to run benchmarks.\n";
        return;
    }
#ifndef NDEBUG
    std::cout << "Use a Release build for meaningful timing results.\n";
#endif
    std::cout << "Comparing both indexes at 10k, 25k, 50k, and 100k events where available.\n";
    const auto results = runBenchmark(data.events, {10000, 25000, 50000, 100000}, 3);
    printResults(results, std::cout);
    std::cout << "Averaged over 3 runs. Autocomplete is Trie-only.\n";
    const auto output = ask("Save results CSV path (blank skips): ");
    if (!output.empty()) {
        writeResultsCsv(results, output);
        std::cout << "Saved " << output << '\n';
    }
}
}

int main(int argc, char* argv[]) {
    if (argc > 2) {
        std::cerr << "Usage: gevents [events.tsv]\n";
        return 1;
    }
    std::unique_ptr<Dataset> data;
    const std::string initialPath = argc == 2 ? argv[1] : "data/events.tsv";
    std::cout << "GEvents\n";
    try {
        data = std::make_unique<Dataset>(initialPath);
        std::cout << "Loaded " << data->events.size() << " events from " << initialPath << '\n';
    } catch (const std::exception& error) {
        std::cout << error.what() << "\nUse Load dataset to select a TSV file.\n";
    }
    bool useTrie = false;
    while (true) {
        try {
            std::cout << "\nIndex: " << (useTrie ? "Trie" : "Hash table")
                      << " | Events: " << (data ? data->events.size() : 0)
                      << "\n1. Load dataset\n2. Search events\n3. Autocomplete (Trie)"
                      << "\n4. Switch index\n5. Run benchmarks\n0. Exit\n";
            const auto choice = ask("Choose: ");
            if (choice == "0") break;
            if (choice == "1") {
                const auto path = ask("TSV path (blank uses data/events.tsv): ");
                auto loaded = std::make_unique<Dataset>(path.empty() ? "data/events.tsv" : path);
                data = std::move(loaded);
                std::cout << "Loaded " << data->events.size() << " events into both indexes.\n";
            } else if (choice == "4") {
                useTrie = !useTrie;
            } else if (choice == "2" || choice == "3" || choice == "5") {
                if (!data) {
                    std::cout << "Load a dataset first.\n";
                    continue;
                }
                if (choice == "2") search(*data, useTrie);
                else if (choice == "3") autocomplete(*data);
                else benchmark(*data);
            } else {
                std::cout << "Choose a menu option from 0 to 5.\n";
            }
        } catch (const std::ios_base::failure&) {
            break;
        } catch (const std::exception& error) {
            std::cout << "Error: " << error.what() << '\n';
        }
    }
    std::cout << "Goodbye.\n";
}
