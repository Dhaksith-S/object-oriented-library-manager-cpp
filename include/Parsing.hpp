#pragma once
#include "Validation.hpp"
#include <charconv>

namespace library {
inline int parseNumber(const std::string& text, int minimum, int maximum) {
    require(!text.empty(), "A number is required.");
    require(std::all_of(text.begin(),text.end(),[](char c){return c>='0' && c<='9';}),
            "Use digits only; negative numbers and signs are not allowed.");
    int value = 0;
    const auto result = std::from_chars(text.data(),text.data()+text.size(),value);
    require(result.ec==std::errc() && result.ptr==text.data()+text.size()
            && value>=minimum && value<=maximum, "Number is outside the allowed range.");
    return value;
}
}
