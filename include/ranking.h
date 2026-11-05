#pragma once

#include "event.h"

struct SearchOptions {
    std::vector<std::string> categories;
    std::vector<std::string> interests;
    std::string startDate;
    std::string endDate;
    std::string referenceDate;
};

struct RankedEvent {
    Event event;
    double score = 0;
};

std::vector<Event> filterEvents(const std::vector<Event>& events,
                                const SearchOptions& options);
std::vector<RankedEvent> rankEvents(const std::vector<Event>& candidates,
                                  const std::vector<std::string>& queryKeywords,
                                  const SearchOptions& options);
