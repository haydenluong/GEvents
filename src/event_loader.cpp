#include "event_loader.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace {

const std::size_t fieldCount = 7;

std::vector<std::string> split(const std::string& line, char separator) {
    std::vector<std::string> parts;
    std::string part;
    std::istringstream stream(line);
    while (std::getline(stream, part, separator)) {
        parts.push_back(part);
    }
    if (!line.empty() && line.back() == separator) {
        parts.emplace_back();
    }
    return parts;
}

std::vector<std::string> splitKeywords(const std::string& field) {
    std::vector<std::string> keywords;
    for (const std::string& keyword : split(field, ';')) {
        if (!keyword.empty()) {
            keywords.push_back(keyword);
        }
    }
    return keywords;
}

Event parseRow(const std::string& line, std::size_t lineNumber) {
    std::vector<std::string> fields = split(line, '\t');
    if (fields.size() != fieldCount) {
        throw std::runtime_error("Line " + std::to_string(lineNumber) + " has " +
                                 std::to_string(fields.size()) + " fields instead of 7");
    }

    Event event;
    try {
        event.id = std::stoi(fields[0]);
    } catch (const std::exception&) {
        throw std::runtime_error("Line " + std::to_string(lineNumber) + " has a bad id");
    }
    event.title = fields[1];
    event.date = fields[2];
    event.location = fields[3];
    event.category = fields[4];
    event.description = fields[5];
    event.keywords = splitKeywords(fields[6]);
    return event;
}

}

std::vector<Event> readEventsFile(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("Could not open " + path);
    }

    std::vector<Event> events;
    std::string line;
    std::size_t lineNumber = 0;
    while (std::getline(file, line)) {
        ++lineNumber;
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (lineNumber == 1 || line.empty()) {
            continue;
        }
        events.push_back(parseRow(line, lineNumber));
    }
    return events;
}
