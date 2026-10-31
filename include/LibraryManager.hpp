#pragma once
#include "Book.hpp"
#include "Member.hpp"
#include "Loan.hpp"
#include <map>
#include <vector>

namespace library {
class LibraryManager {
    std::map<int, Book> books_;
    std::map<int, Member> members_;
    std::map<int, Loan> loans_;
public:
    static constexpr std::size_t loanLimit = 3;
    static constexpr int loanDays = 14;
    const std::map<int, Book>& books() const { return books_; }
    const std::map<int, Member>& members() const { return members_; }
    const std::map<int, Loan>& loans() const { return loans_; }
    void addBook(Book book);
    void addMember(Member member);
    int registerMember(std::string name, std::string contact);
    std::vector<int> search(const std::string& field, const std::string& query) const;
    std::size_t activeForBook(int bookId) const;
    std::size_t activeForMember(int memberId) const;
    int available(int bookId) const;
    int issue(int bookId, int memberId, Date date = Date::today());
    void returnBook(int loanId, Date date = Date::today());
    void validate() const;
    static LibraryManager fromRecords(std::vector<Book> books, std::vector<Member> members,
                                      std::vector<Loan> loans);
    // Identical IDs are a no-op; conflicting IDs reject the entire import.
    bool merge(const LibraryManager& incoming);
};
}
