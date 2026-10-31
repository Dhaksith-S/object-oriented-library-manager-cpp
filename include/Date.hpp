#pragma once
#include <string>

namespace library {
// Gregorian calendar dates from 1900-01-01 to 9999-12-31, independent of DST.
class Date {
    int year_, month_, day_;
public:
    Date(int year, int month, int day);
    static Date parse(const std::string& iso);
    static Date today(); // UTC calendar date
    Date plusDays(int days) const;
    std::string str() const;
    friend bool operator<(const Date& a, const Date& b);
};
}
