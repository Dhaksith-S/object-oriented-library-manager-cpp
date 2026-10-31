#include "Member.hpp"
#include "Validation.hpp"
#include <utility>

namespace library {
Member::Member(int id, std::string name, std::string contact, bool active)
    : id_(id), name_(std::move(name)), contact_(std::move(contact)), active_(active) {
    validId(id_); validText(name_); validText(contact_);
}
bool Member::eligible(std::size_t activeLoans, std::size_t limit) const {
    return active_ && activeLoans < limit;
}
}
