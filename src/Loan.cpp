#include "Loan.hpp"
#include "Validation.hpp"

namespace library {
Loan::Loan(int id, int bookId, int memberId, Date issued, Date due, std::optional<Date> returned)
    : id_(id), bookId_(bookId), memberId_(memberId), issued_(issued), due_(due), returned_(returned) {
    validId(id_); validId(bookId_); validId(memberId_);
    require(!(due_ < issued_), "Due date precedes issue date.");
    require(!returned_ || !(*returned_ < issued_), "Return date precedes issue date.");
}
void Loan::returnOn(Date date) {
    require(active(), "Loan has already been returned.");
    require(!(date < issued_), "Return date precedes issue date.");
    returned_ = date;
}
}
