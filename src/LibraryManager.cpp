#include "LibraryManager.hpp"
#include "Validation.hpp"
#include <algorithm>
#include <utility>

namespace library {
namespace {
template<class T> int nextId(const std::map<int, T>& records) {
    require(records.size() < maxRecords, "Record limit reached.");
    if (records.empty()) return 1;
    require(records.rbegin()->first < maxId, "ID space exhausted.");
    return records.rbegin()->first + 1;
}
std::string folded(std::string value) {
    for (char& c : value) if (c >= 'A' && c <= 'Z') c = static_cast<char>(c + ('a' - 'A'));
    return value;
}
bool same(const Book& a, const Book& b) {
    return a.title()==b.title() && a.author()==b.author() && a.genre()==b.genre() && a.copies()==b.copies();
}
bool same(const Member& a, const Member& b) {
    return a.name()==b.name() && a.contact()==b.contact() && a.active()==b.active();
}
bool same(const Loan& a, const Loan& b) {
    return a.bookId()==b.bookId() && a.memberId()==b.memberId() && a.issued().str()==b.issued().str()
        && a.due().str()==b.due().str() && a.active()==b.active()
        && (a.active() || a.returned()->str()==b.returned()->str());
}
template<class T> bool mergeRecords(std::map<int,T>& target, const std::map<int,T>& incoming) {
    bool changed = false;
    for (const auto& entry : incoming) {
        const auto old = target.find(entry.first);
        if (old != target.end()) require(same(old->second, entry.second), "Import conflicts with existing ID.");
        else {
            require(target.size() < maxRecords, "Record limit reached.");
            target.emplace(entry); changed = true;
        }
    }
    return changed;
}
}
void LibraryManager::addBook(Book book) {
    require(books_.size() < maxRecords, "Book record limit reached.");
    require(books_.emplace(book.id(), std::move(book)).second, "Duplicate book ID.");
}
void LibraryManager::addMember(Member member) {
    require(members_.size() < maxRecords, "Member record limit reached.");
    require(members_.emplace(member.id(), std::move(member)).second, "Duplicate member ID.");
}
int LibraryManager::registerMember(std::string name, std::string contact) {
    const int id = nextId(members_);
    addMember(Member(id, std::move(name), std::move(contact)));
    return id;
}
std::vector<int> LibraryManager::search(const std::string& field, const std::string& query) const {
    require(field=="title" || field=="author" || field=="id" || field=="genre", "Unknown search field.");
    validText(query);
    std::vector<int> result;
    for (const auto& [id,b] : books_) {
        const std::string text = field=="title" ? b.title() : field=="author" ? b.author()
                               : field=="genre" ? b.genre() : std::to_string(id);
        if ((field=="id" && text==query) || (field!="id" && folded(text).find(folded(query))!=std::string::npos))
            result.push_back(id);
    }
    return result;
}
std::size_t LibraryManager::activeForBook(int bookId) const {
    require(books_.count(bookId)!=0, "Unknown book ID.");
    return static_cast<std::size_t>(std::count_if(loans_.begin(),loans_.end(),[bookId](const auto& p) {
        return p.second.active() && p.second.bookId()==bookId;
    }));
}
std::size_t LibraryManager::activeForMember(int memberId) const {
    require(members_.count(memberId)!=0, "Unknown member ID.");
    return static_cast<std::size_t>(std::count_if(loans_.begin(),loans_.end(),[memberId](const auto& p) {
        return p.second.active() && p.second.memberId()==memberId;
    }));
}
int LibraryManager::available(int bookId) const {
    const auto count = activeForBook(bookId);
    return books_.at(bookId).available(count);
}
int LibraryManager::issue(int bookId, int memberId, Date date) {
    require(books_.count(bookId)!=0, "Unknown book ID.");
    require(members_.count(memberId)!=0, "Unknown member ID.");
    require(members_.at(memberId).active(), "Member is inactive.");
    require(available(bookId)>0, "Book unavailable: no copies remain.");
    require(members_.at(memberId).eligible(activeForMember(memberId),loanLimit), "Member borrowing limit reached (3).");
    const int id = nextId(loans_);
    loans_.emplace(id, Loan(id,bookId,memberId,date,date.plusDays(loanDays)));
    return id;
}
void LibraryManager::returnBook(int loanId, Date date) {
    const auto it = loans_.find(loanId);
    require(it!=loans_.end(), "Unknown loan ID.");
    it->second.returnOn(date);
}
void LibraryManager::validate() const {
    require(books_.size()<=maxRecords && members_.size()<=maxRecords && loans_.size()<=maxRecords,
            "Record limit exceeded.");
    std::map<int,std::size_t> bookActive, memberActive;
    for (const auto& [id,l] : loans_) {
        (void)id;
        require(books_.count(l.bookId()) && members_.count(l.memberId()), "Loan references missing book/member.");
        if (l.active()) {
            require(members_.at(l.memberId()).active(), "Active loan belongs to inactive member.");
            ++bookActive[l.bookId()]; ++memberActive[l.memberId()];
        }
    }
    for (const auto& [id,count] : bookActive) (void)books_.at(id).available(count);
    for (const auto& [id,count] : memberActive) { (void)id; require(count<=loanLimit, "Member loan limit exceeded."); }
}
LibraryManager LibraryManager::fromRecords(std::vector<Book> books, std::vector<Member> members,
                                          std::vector<Loan> loans) {
    LibraryManager result;
    for (auto& b : books) result.addBook(std::move(b));
    for (auto& m : members) result.addMember(std::move(m));
    for (auto& l : loans) {
        require(result.loans_.size()<maxRecords, "Loan record limit exceeded.");
        require(result.loans_.emplace(l.id(),std::move(l)).second, "Duplicate loan ID.");
    }
    result.validate(); return result;
}
bool LibraryManager::merge(const LibraryManager& incoming) {
    auto candidate = *this;
    bool changed = mergeRecords(candidate.books_,incoming.books_);
    changed = mergeRecords(candidate.members_,incoming.members_) || changed;
    changed = mergeRecords(candidate.loans_,incoming.loans_) || changed;
    candidate.validate();
    *this = std::move(candidate); return changed;
}
}
