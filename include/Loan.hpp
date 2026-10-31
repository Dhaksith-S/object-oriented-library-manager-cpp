#pragma once
#include "Date.hpp"
#include <optional>

namespace library {
class Loan {
    int id_, bookId_, memberId_;
    Date issued_, due_;
    std::optional<Date> returned_;
public:
    Loan(int id, int bookId, int memberId, Date issued, Date due,
         std::optional<Date> returned = std::nullopt);
    int id() const { return id_; }
    int bookId() const { return bookId_; }
    int memberId() const { return memberId_; }
    const Date& issued() const { return issued_; }
    const Date& due() const { return due_; }
    const std::optional<Date>& returned() const { return returned_; }
    bool active() const { return !returned_.has_value(); }
    void returnOn(Date date);
};
}
