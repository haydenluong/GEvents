#pragma once

#include <cstddef>
#include <string>
#include <vector>

class Index {
public:
    virtual ~Index() = default;

    virtual void insert(const std::string& keyword, int eventId) = 0;
    virtual std::vector<int> find(const std::string& keyword) const = 0;
    virtual std::size_t memoryBytes() const = 0;
};
