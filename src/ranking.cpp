#include "ranking.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace {
int dayNumber(const std::string& date) {
    if (date.size() != 10 || date[4] != '-' || date[7] != '-') {
        throw std::invalid_argument("Dates must use YYYY-MM-DD");
    }
    for (std::size_t i = 0; i < date.size(); ++i) {
        if (i != 4 && i != 7 && (date[i] < '0' || date[i] > '9')) {
            throw std::invalid_argument("Dates must use YYYY-MM-DD");
        }
    }
    int year = std::stoi(date.substr(0, 4));
    const int month = std::stoi(date.substr(5, 2));
    const int day = std::stoi(date.substr(8, 2));
    const bool leap = year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
    const int monthDays[] = {31, 28 + leap, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (year < 1 || month < 1 || month > 12 || day < 1 || day > monthDays[month - 1]) {
        throw std::invalid_argument("Invalid calendar date");
    }
    year -= month <= 2;
    const int era = year / 400;
    const int yearOfEra = year - era * 400;
    const int adjustedMonth = month + (month > 2 ? -3 : 9);
    const int dayOfYear = (153 * adjustedMonth + 2) / 5 + day - 1;
    return era * 146097 + yearOfEra * 365 + yearOfEra / 4 - yearOfEra / 100 + dayOfYear;
}

bool contains(const std::vector<std::string>& values, const std::string& value) {
    return std::find(values.begin(), values.end(), value) != values.end();
}
}

std::vector<Event> filterEvents(const std::vector<Event>& events,
                                const SearchOptions& options) {
    if (!options.startDate.empty()) dayNumber(options.startDate);
    if (!options.endDate.empty()) dayNumber(options.endDate);
    if (!options.startDate.empty() && !options.endDate.empty()
        && options.startDate > options.endDate) {
        throw std::invalid_argument("Start date must not follow end date");
    }
    std::vector<Event> result;
    for (const auto& event : events) {
        dayNumber(event.date);
        if (!options.categories.empty() && !contains(options.categories, event.category)) continue;
        if (!options.startDate.empty() && event.date < options.startDate) continue;
        if (!options.endDate.empty() && event.date > options.endDate) continue;
        result.push_back(event);
    }
    return result;
}

std::vector<RankedEvent> rankEvents(const std::vector<Event>& candidates,
                                  const std::vector<std::string>& queryKeywords,
                                  const SearchOptions& options) {
    const bool useDateBonus = !options.referenceDate.empty();
    const int reference = useDateBonus ? dayNumber(options.referenceDate) : 0;
    auto keywords = queryKeywords;
    std::sort(keywords.begin(), keywords.end());
    keywords.erase(std::unique(keywords.begin(), keywords.end()), keywords.end());
    std::vector<RankedEvent> result;
    for (const auto& event : filterEvents(candidates, options)) {
        std::size_t matched = 0;
        for (const auto& keyword : keywords) {
            if (contains(event.keywords, keyword)) ++matched;
        }
        if (matched != keywords.size()) continue;
        const double dateBonus = useDateBonus
            ? 1.0 / (1.0 + std::abs(dayNumber(event.date) - reference)) : 0.0;
        const double score = 3.0 * matched
            + (contains(options.interests, event.category) ? 2.0 : 0.0) + dateBonus;
        result.push_back({event, score});
    }
    std::sort(result.begin(), result.end(), [](const RankedEvent& a, const RankedEvent& b) {
        if (a.score != b.score) return a.score > b.score;
        if (a.event.date != b.event.date) return a.event.date < b.event.date;
        return a.event.id < b.event.id;
    });
    return result;
}
