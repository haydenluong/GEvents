#include "query.h"



std::vector<std::string> parseQuery(const std::string& query) {
    std::vector<std::string> keywords;
    std::string keyword;

    for (char letter : query) {
        if (letter == ' ' || letter == '\t' || letter == '\n' || letter == '\r' || letter == '\f' || letter == '\v') {
            if (!keyword.empty()) {
                keywords.push_back(keyword);
                keyword.clear();
            }
        } else {
            if (letter >= 'A' && letter <= 'Z') {
                letter += 'a' - 'A';
            }
            keyword += letter;
        }
    }

    if (!keyword.empty()) {
        keywords.push_back(keyword);
    }

    return keywords;
}

std::vector<int> findMatchingEvents(const Index& index, const std::vector<std::string>& keywords) {
    if (keywords.empty()) {
        return {};
    }

    std::vector<int> matches = index.find(keywords[0]);

    for (std::size_t word = 1; word < keywords.size() && !matches.empty(); word++) {
        std::vector<int> eventIds = index.find(keywords[word]);
        std::vector<int> sharedIds;
        std::size_t left = 0;
        std::size_t right = 0;

        while (left < matches.size() && right < eventIds.size()) {
            if (matches[left] == eventIds[right]) {
                sharedIds.push_back(matches[left]);
                left++;
                right++;

            } else if (matches[left] < eventIds[right]) {
                left++;
            } else {
                right++;
            }
        }

        matches.swap(sharedIds);
    }

    return matches;
}
