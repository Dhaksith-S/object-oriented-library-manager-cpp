#include "Book.hpp"
#include "Validation.hpp"
#include <utility>

namespace library {
Book::Book(int id, std::string title, std::string author, std::string genre, int copies)
    : id_(id), title_(std::move(title)), author_(std::move(author)),
      genre_(std::move(genre)), copies_(copies) {
    validId(id_); validText(title_); validText(author_); validText(genre_);
    require(copies >= 0 && copies <= maxCopies, "Copies must be between 0 and 1000000.");
}
int Book::available(std::size_t activeLoans) const {
    require(activeLoans <= static_cast<std::size_t>(copies_), "Active loans exceed book inventory.");
    return copies_ - static_cast<int>(activeLoans);
}
}
