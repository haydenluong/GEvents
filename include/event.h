#pragma once

#include <string>
#include <vector>

struct Event {
    int id = 0;
    std::string title;
    std::string date;
    std::string location;
    std::string category;
    std::string description;
    std::vector<std::string> keywords;
};
