#pragma once
#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string>

namespace library {
constexpr int maxId = 1000000000;
constexpr int maxCopies = 1000000;
constexpr std::size_t maxRecords = 100000;
inline void require(bool condition, const std::string& message) {
    if (!condition) throw std::invalid_argument(message);
}
inline void validId(int id) { require(id > 0 && id <= maxId, "ID must be between 1 and 1000000000."); }
inline void validText(const std::string& value) {
    require(!value.empty() && value.size() <= 512, "Text must contain 1 to 512 bytes.");
    require(std::any_of(value.begin(), value.end(), [](unsigned char c) { return !std::isspace(c); }),
            "Text cannot be blank.");
    require(std::none_of(value.begin(), value.end(), [](unsigned char c) {
        return (c < 32 && c != '\n' && c != '\r' && c != '\t') || c == 127;
    }), "Text contains an unsupported control character.");
}
}
