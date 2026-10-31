# UML class diagram

This diagram follows the public APIs and private state in `include/`.
Only the manager owns entities; loans store IDs, not raw pointers or copies of members/books.

```mermaid
classDiagram
direction LR
class Book {
  -int id_
  -string title_
  -string author_
  -string genre_
  -int copies_
  +Book(id, title, author, genre, copies)
  +id() int
  +title() string
  +author() string
  +genre() string
  +copies() int
  +available(activeLoans) int
}
class Member {
  -int id_
  -string name_
  -string contact_
  -bool active_
  +Member(id, name, contact, active)
  +id() int
  +name() string
  +contact() string
  +active() bool
  +eligible(activeLoans, limit) bool
}
class Loan {
  -int id_
  -int bookId_
  -int memberId_
  -Date issued_
  -Date due_
  -optional~Date~ returned_
  +Loan(id, bookId, memberId, issued, due, returned)
  +id() int
  +bookId() int
  +memberId() int
  +issued() Date
  +due() Date
  +returned() optional~Date~
  +active() bool
  +returnOn(date) void
}
class LibraryManager {
  -map books_
  -map members_
  -map loans_
  +loanLimit = 3
  +loanDays = 14
  +books() map
  +members() map
  +loans() map
  +addBook(book) void
  +addMember(member) void
  +registerMember(name, contact) int
  +search(field, query) vector~int~
  +activeForBook(bookId) size_t
  +activeForMember(memberId) size_t
  +available(bookId) int
  +issue(bookId, memberId, date) int
  +returnBook(loanId, date) void
  +validate() void
  +fromRecords(books, members, loans) LibraryManager$
  +merge(incoming) bool
}
class StorageManager {
  +readSnapshot(path) LibraryManager$
  +load(path) LoadResult$
  +save(library, path) void$
  +serialize(library) string$
}
class LoadResult {
  +LibraryManager library
  +string notice
}
class Date {
  -int year_
  -int month_
  -int day_
  +Date(year, month, day)
  +parse(iso) Date$
  +today() Date$
  +plusDays(days) Date
  +str() string
}
LibraryManager "1" *-- "0..*" Book : owns
LibraryManager "1" *-- "0..*" Member : owns
LibraryManager "1" *-- "0..*" Loan : owns
Loan "0..*" --> "1" Book : bookId
Loan "0..*" --> "1" Member : memberId
Loan "1" *-- "2..3" Date : issue, due, optional return
StorageManager ..> LibraryManager : validates and persists
StorageManager ..> LoadResult : returns
LoadResult "1" *-- "1" LibraryManager : recovered state
```

`$` denotes a static method. String/date/map getters in C++ return const references
where applicable; the diagram abbreviates these signatures for readability.
Collection multiplicities describe logical ownership within each library instance.
Book/member associations are enforced by manager validation, including for returned
loans. Dates are value objects; a loan owns two required dates and an optional return
date. The free `operator<` compares Date values. `main.cpp` orchestrates the manager
and storage without introducing a domain dependency on terminal interaction.
