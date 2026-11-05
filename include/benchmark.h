#pragma once

#include "event.h"

#include <cstddef>
#include <ostream>
#include <string>
#include <vector>

struct BenchmarkResult {
    std::string structure;
    std::size_t events = 0;
    std::size_t queries = 0;
    double insertMs = 0;
    double searchUs = 0;
    std::size_t memoryBytes = 0;
    std::size_t matches = 0;
};

std::vector<BenchmarkResult> runBenchmark(const std::vector<Event>& events,
                                          const std::vector<std::size_t>& sizes, int repeats);
void printResults(const std::vector<BenchmarkResult>& results, std::ostream& out);
void writeResultsCsv(const std::vector<BenchmarkResult>& results, const std::string& path);
