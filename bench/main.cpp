#include "benchmark.h"
#include "event_loader.h"

#include <iostream>
#include <stdexcept>
#include <string>

namespace {

struct Options {
    std::string dataPath = "data/events.tsv";
    std::string csvPath = "data/benchmark.csv";
    int repeats = 5;
};

Options parseOptions(int argc, char* argv[]) {
    Options options;
    for (int i = 1; i < argc; i += 2) {
        std::string flag = argv[i];
        if (i + 1 >= argc) {
            throw std::invalid_argument("Missing value for " + flag);
        }
        std::string value = argv[i + 1];
        if (flag == "--data") {
            options.dataPath = value;
        } else if (flag == "--csv") {
            options.csvPath = value;
        } else if (flag == "--repeats") {
            options.repeats = std::stoi(value);
        } else {
            throw std::invalid_argument("Unknown option " + flag);
        }
    }
    return options;
}

}

int main(int argc, char* argv[]) {
    try {
        Options options = parseOptions(argc, argv);
#ifndef NDEBUG
        std::cerr << "Warning: this build is not optimized. Use -DCMAKE_BUILD_TYPE=Release for real numbers.\n";
#endif
        std::vector<Event> events = readEventsFile(options.dataPath);
        std::cout << "Loaded " << events.size() << " events from " << options.dataPath << "\n\n";

        auto results = runBenchmark(events, {10000, 25000, 50000, 100000}, options.repeats);
        if (results.empty()) {
            std::cerr << "Need at least 10000 events to benchmark\n";
            return 1;
        }
        printResults(results, std::cout);
        writeResultsCsv(results, options.csvPath);
        std::cout << "\nAveraged over " << options.repeats << " runs. Saved to " << options.csvPath << '\n';
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
