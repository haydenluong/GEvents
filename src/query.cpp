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
