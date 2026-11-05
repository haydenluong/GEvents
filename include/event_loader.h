#pragma once

#include "event.h"

#include <string>
#include <vector>

std::vector<Event> readEventsFile(const std::string& path);
