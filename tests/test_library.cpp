#include "Test.hpp"
#include "LibraryManager.hpp"
#include "StorageManager.hpp"
#include "Validation.hpp"

using namespace library;
int main(int argc,char** argv) { return runTests([&] {
    CHECK(argc==2);
    const auto date=Date(2026,1,1);
    LibraryManager lib;
    CHECK(lib.books().empty() && lib.members().empty() && lib.loans().empty());
    lib.addBook(Book(1,"Bright River","Mira Reed","Fiction",4));
    lib.addBook(Book(2,"Quiet Sky","Iris Reed","Science",1));
    lib.addBook(Book(3,"Empty Shelf","Iris West","Science",0));
    CHECK(lib.books().size()==3 && lib.available(1)==4);
    rejects([&]{ lib.addBook(Book(1,"Duplicate","A","B",1)); });
    CHECK(lib.registerMember("Alex","alex@example.invalid")==1);
    lib.addMember(Member(8,"Inactive","contact",false));
    CHECK(lib.registerMember("Blair","blair@example.invalid")==9);
    rejects([&]{ lib.addMember(Member(1,"Duplicate","contact")); });
    CHECK(lib.search("title","bRIGHT")==std::vector<int>{1});
    CHECK(lib.search("author","REED").size()==2);
    CHECK(lib.search("genre","sciENCE").size()==2);
    CHECK(lib.search("id","1")==std::vector<int>{1});
    CHECK(lib.search("id","10").empty() && lib.search("title","missing").empty());
    rejects([&]{ (void)lib.search("unknown","x"); });
    rejects([&]{ (void)lib.search("title",""); });
    rejects([&]{ (void)lib.issue(99,1,date); });
    rejects([&]{ (void)lib.issue(1,99,date); });
    rejects([&]{ (void)lib.issue(1,8,date); });
    rejects([&]{ (void)lib.issue(3,1,date); });
    CHECK(lib.issue(1,1,date)==1);
    CHECK(lib.available(1)==3 && lib.activeForMember(1)==1);
    CHECK(lib.loans().at(1).due().str()=="2026-01-15");
    CHECK(lib.issue(1,1,date)==2 && lib.issue(1,1,date)==3);
    rejects([&]{ (void)lib.issue(1,1,date); });
    CHECK(lib.available(1)==1 && lib.activeForMember(1)==3 && lib.loans().size()==3);
    CHECK(lib.issue(2,9,date)==4);
    rejects([&]{ (void)lib.issue(2,1,date); });
    rejects([&]{ (void)lib.available(-1); });
    rejects([&]{ (void)lib.activeForMember(99); });
    rejects([&]{ lib.returnBook(99,date); });
    rejects([&]{ lib.returnBook(1,date.plusDays(-1)); });
    CHECK(lib.activeForMember(1)==3);
    lib.returnBook(1,date.plusDays(1));
    CHECK(lib.available(1)==2 && lib.activeForMember(1)==2 && lib.loans().at(2).active());
    rejects([&]{ lib.returnBook(1,date.plusDays(2)); });
    CHECK(lib.issue(1,1,date.plusDays(2))==5);
    const auto before=StorageManager::serialize(lib);
    rejects([&]{ (void)lib.issue(1,9,Date(9999,12,31)); });
    CHECK(StorageManager::serialize(lib)==before);
    lib.validate();
    auto sample=StorageManager::readSnapshot(argv[1]);
    CHECK(sample.books().size()==15 && sample.members().size()==8 && sample.loans().size()==10);
    CHECK(sample.available(7)==1 && sample.activeForMember(1)==1);
    LibraryManager imported;
    CHECK(imported.merge(sample));
    const auto original=StorageManager::serialize(imported);
    CHECK(!imported.merge(sample) && StorageManager::serialize(imported)==original);
    rejects([&]{ imported.merge(lib); });
    CHECK(StorageManager::serialize(imported)==original);
    // A returned sample loan conflicts on re-import rather than reactivating silently.
    imported.returnBook(7,Date(2026,2,1));
    const auto returned=StorageManager::serialize(imported);
    rejects([&]{ imported.merge(sample); });
    CHECK(StorageManager::serialize(imported)==returned);
    LibraryManager exhausted;
    exhausted.addMember(Member(maxId,"Last","contact"));
    rejects([&]{ (void)exhausted.registerMember("New","contact"); });
    auto maxLoan=LibraryManager::fromRecords({Book(1,"A","B","C",2)}, {Member(1,"A","B")},
        {Loan(maxId,1,1,date,date)});
    rejects([&]{ (void)maxLoan.issue(1,1,date); });
    rejects([&]{ (void)LibraryManager::fromRecords({}, {}, {Loan(1,1,1,date,date)}); });
    rejects([&]{ (void)LibraryManager::fromRecords({Book(1,"A","B","C",0)}, {Member(1,"A","B")},
        {Loan(1,1,1,date,date)}); });
    rejects([&]{ (void)LibraryManager::fromRecords({Book(1,"A","B","C",1)}, {Member(1,"A","B",false)},
        {Loan(1,1,1,date,date)}); });
    rejects([&]{ (void)LibraryManager::fromRecords({Book(1,"A","B","C",4)}, {Member(1,"A","B")},
        {Loan(1,1,1,date,date),Loan(2,1,1,date,date),Loan(3,1,1,date,date),Loan(4,1,1,date,date)}); });
    // Capacity overflow during a merge must not partially insert any records.
    auto target=LibraryManager::fromRecords({Book(1,"A","B","C",1)}, {Member(1,"A","B")}, {Loan(1,1,1,date,date)});
    auto extra=LibraryManager::fromRecords({Book(1,"A","B","C",1)}, {Member(2,"C","D")}, {Loan(2,1,2,date,date)});
    const auto targetBefore=StorageManager::serialize(target);
    rejects([&]{ target.merge(extra); });
    CHECK(StorageManager::serialize(target)==targetBefore);
}); }
