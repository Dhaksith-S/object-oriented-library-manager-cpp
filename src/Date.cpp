#include "Date.hpp"
#include "Validation.hpp"
#include <ctime>
#include <iomanip>
#include <sstream>

namespace library {
namespace {
bool leap(int year) { return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0); }
int monthDays(int year, int month) {
    constexpr int days[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    return days[month - 1] + ((month == 2 && leap(year)) ? 1 : 0);
}
long ordinal(int year, int month, int day) {
    const long y = year - 1;
    long n = y * 365 + y / 4 - y / 100 + y / 400 + day - 1;
    for (int m = 1; m < month; ++m) n += monthDays(year, m);
    return n;
}
}
Date::Date(int year, int month, int day) : year_(year), month_(month), day_(day) {
    require(year >= 1900 && year <= 9999 && month >= 1 && month <= 12,
            "Date year/month is outside supported range (1900-9999).");
    require(day >= 1 && day <= monthDays(year, month), "Invalid calendar day.");
}
Date Date::parse(const std::string& iso) {
    require(iso.size() == 10 && iso[4] == '-' && iso[7] == '-', "Date must use YYYY-MM-DD.");
    for (std::size_t i = 0; i < iso.size(); ++i)
        if (i != 4 && i != 7) require(iso[i] >= '0' && iso[i] <= '9', "Invalid date digit.");
    return Date(std::stoi(iso.substr(0,4)), std::stoi(iso.substr(5,2)), std::stoi(iso.substr(8,2)));
}
Date Date::today() {
    const auto now = std::time(nullptr);
    const auto* utc = std::gmtime(&now);
    if (!utc) throw std::runtime_error("Cannot determine UTC calendar date.");
    return Date(utc->tm_year + 1900, utc->tm_mon + 1, utc->tm_mday);
}
Date Date::plusDays(int days) const {
    const long long result = static_cast<long long>(ordinal(year_, month_, day_)) + days;
    require(result >= ordinal(1900,1,1) && result <= ordinal(9999,12,31), "Date arithmetic overflow.");
    int low = 1900, high = 9999;
    while (low < high) {
        const int mid = low + (high - low + 1) / 2;
        if (ordinal(mid,1,1) <= result) low = mid; else high = mid - 1;
    }
    int month = 1;
    auto remaining = result - ordinal(low,1,1);
    while (remaining >= monthDays(low, month)) remaining -= monthDays(low, month++);
    return Date(low, month, static_cast<int>(remaining) + 1);
}
std::string Date::str() const {
    std::ostringstream out;
    out << std::setfill('0') << std::setw(4) << year_ << '-' << std::setw(2) << month_ << '-'
        << std::setw(2) << day_;
    return out.str();
}
bool operator<(const Date& a, const Date& b) {
    return ordinal(a.year_, a.month_, a.day_) < ordinal(b.year_, b.month_, b.day_);
}
}
