#pragma once

#include "index.h"

#include <string>
#include <vector>

std::vector<std::string> parseQuery(const std::string& query);
std::vector<int> findMatchingEvents(const Index& index, const std::vector<std::string>& keywords);
