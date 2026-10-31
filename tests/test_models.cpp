#include "Test.hpp"
#include "Book.hpp"
#include "Member.hpp"
#include "Loan.hpp"
#include "Parsing.hpp"
#include <climits>

using namespace library;
int main() { return runTests([] {
    Book book(1,"A, \"Book\"","Author","Genre",2);
    CHECK(book.id()==1 && book.copies()==2 && book.title()=="A, \"Book\"");
    CHECK(book.available(0)==2 && book.available(2)==0);
    CHECK(Book(2,"A","B","C",0).available(0)==0);
    CHECK(Book(maxId,"A","B","C",maxCopies).copies()==maxCopies);
    rejects([&]{ (void)book.available(3); });
    rejects([]{ Book b(0,"A","B","C",1); });
    rejects([]{ Book b(-1,"A","B","C",1); });
    rejects([]{ Book b(maxId+1,"A","B","C",1); });
    rejects([]{ Book b(1,"","B","C",1); });
    rejects([]{ Book b(1,"A","  ","C",1); });
    rejects([]{ Book b(1,"A","B","",1); });
    rejects([]{ Book b(1,"A","B","C",-1); });
    rejects([]{ Book b(1,"A","B","C",maxCopies+1); });
    rejects([]{ Book b(1,std::string(513,'x'),"B","C",1); });
    rejects([]{ Book b(1,std::string("a\0b",3),"B","C",1); });
    CHECK(Book(1,std::string(512,'x'),"B","C",1).title().size()==512);
    Member m(1,"Example","example@example.invalid");
    CHECK(m.id()==1 && m.active() && m.name()=="Example" && !m.contact().empty());
    CHECK(m.eligible(0,3) && !m.eligible(3,3));
    CHECK(!Member(2,"Other","contact",false).eligible(0,3));
    rejects([]{ Member invalid(0,"A","B"); });
    rejects([]{ Member invalid(1,"","B"); });
    rejects([]{ Member invalid(1,"A","\t"); });
    const auto day=Date::parse("2024-02-28");
    CHECK(day.plusDays(1).str()=="2024-02-29");
    CHECK(day.plusDays(2).str()=="2024-03-01");
    CHECK(Date(1900,2,28).plusDays(1).str()=="1900-03-01");
    CHECK(Date(2000,2,28).plusDays(1).str()=="2000-02-29");
    CHECK(Date(2025,12,31).plusDays(1).str()=="2026-01-01");
    CHECK(Date(2026,1,1).plusDays(-1).str()=="2025-12-31");
    CHECK(Date::today().str().size()==10);
    for(const auto& text:{"2025-02-29","1900-02-29","2026-00-01","2026-13-01","2026-01-00",
                          "2026-04-31","1899-12-31","10000-01-01","2026-1-01","202x-01-01",""})
        rejects([&]{ (void)Date::parse(text); });
    rejects([]{ (void)Date(9999,12,31).plusDays(1); });
    rejects([]{ (void)Date(1900,1,1).plusDays(-1); });
    rejects([]{ (void)Date(2026,1,1).plusDays(INT_MAX); });
    rejects([]{ (void)Date(2026,1,1).plusDays(INT_MIN); });
    // A complete Gregorian 400-year cycle, including century exceptions.
    for(int delta=0;delta<=146097;++delta) {
        const auto date=Date(2000,1,1).plusDays(delta);
        CHECK(date.plusDays(-delta).str()=="2000-01-01");
        CHECK(Date::parse(date.str()).str()==date.str());
    }
    CHECK(Date(2000,1,1).plusDays(146097).str()=="2400-01-01");
    Loan loan(1,2,3,day,day.plusDays(14));
    CHECK(loan.active() && !loan.returned() && loan.bookId()==2 && loan.memberId()==3);
    CHECK(loan.issued().str()=="2024-02-28" && loan.due().str()=="2024-03-13");
    rejects([&]{ loan.returnOn(day.plusDays(-1)); });
    CHECK(loan.active());
    loan.returnOn(day.plusDays(20)); // Late returns remain valid.
    CHECK(!loan.active() && loan.returned()->str()=="2024-03-19");
    rejects([&]{ loan.returnOn(day.plusDays(21)); });
    rejects([&]{ Loan invalid(0,2,3,day,day); });
    rejects([&]{ Loan invalid(1,0,3,day,day); });
    rejects([&]{ Loan invalid(1,2,0,day,day); });
    rejects([&]{ Loan invalid(1,2,3,day,day.plusDays(-1)); });
    rejects([&]{ Loan invalid(1,2,3,day,day,day.plusDays(-1)); });
    for(const auto& value:{"","abc","-1","+1"," 1","1 ","1x","999999999999999999999999"})
        rejects([&]{ (void)parseNumber(value,0,maxId); });
    CHECK(parseNumber("0",0,10)==0 && parseNumber("1000000000",1,maxId)==maxId);
}); }
