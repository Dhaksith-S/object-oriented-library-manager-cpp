#pragma once
#include <string>

namespace library {
class Book {
    int id_;
    std::string title_, author_, genre_;
    int copies_;
public:
    Book(int id, std::string title, std::string author, std::string genre, int copies);
    int id() const { return id_; }
    const std::string& title() const { return title_; }
    const std::string& author() const { return author_; }
    const std::string& genre() const { return genre_; }
    int copies() const { return copies_; }
    int available(std::size_t activeLoans) const;
};
}
