#pragma once
#include <string>

namespace library {
class Member {
    int id_;
    std::string name_, contact_;
    bool active_;
public:
    Member(int id, std::string name, std::string contact, bool active = true);
    int id() const { return id_; }
    const std::string& name() const { return name_; }
    const std::string& contact() const { return contact_; }
    bool active() const { return active_; }
    bool eligible(std::size_t activeLoans, std::size_t limit) const;
};
}
