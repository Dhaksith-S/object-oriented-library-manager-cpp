#include "Test.hpp"
#include "StorageManager.hpp"

using namespace library;
int main(int argc,char** argv) { return runTests([&] {
    CHECK(argc==2);
    TempDirectory temp;
    const auto db=temp.path/"library.csv";
    auto fresh=StorageManager::load(db);
    CHECK(fresh.library.books().empty() && fresh.notice.find("empty")!=std::string::npos);
    rejects([&]{ (void)StorageManager::readSnapshot(db); });
    auto lib=StorageManager::readSnapshot(argv[1]);
    const auto original=StorageManager::serialize(lib);
    StorageManager::save(lib,db);
    CHECK(StorageManager::serialize(StorageManager::load(db).library)==original);
    CHECK(StorageManager::load(db).library.loans().at(7).active());
    CHECK(!std::filesystem::exists(db.string()+".bak"));
    lib.addBook(Book(16,"Quoted \"title\", with comma\nand newline\r\nnext line","Author\tName","Category",2));
    StorageManager::save(lib,db);
    const auto current=StorageManager::serialize(lib);
    CHECK(StorageManager::serialize(StorageManager::load(db).library)==current);
    CHECK(StorageManager::readSnapshot(db).books().at(16).title()==lib.books().at(16).title());
    CHECK(StorageManager::serialize(StorageManager::readSnapshot(db.string()+".bak"))==original);
    writeFile(db,"broken snapshot\n");
    const auto recovered=StorageManager::load(db);
    CHECK(recovered.notice.find("recovered")!=std::string::npos);
    CHECK(StorageManager::serialize(recovered.library)==original);
    StorageManager::save(recovered.library,db);
    CHECK(readFile(db.string()+".corrupt")=="broken snapshot\n");
    CHECK(StorageManager::serialize(StorageManager::load(db).library)==original);
    std::filesystem::remove(db);
    CHECK(StorageManager::serialize(StorageManager::load(db).library)==original);
    writeFile(db,"bad\n"); writeFile(db.string()+".bak","bad backup\n");
    rejects([&]{ (void)StorageManager::load(db); });
    CHECK(readFile(db)=="bad\n" && readFile(db.string()+".bak")=="bad backup\n");
    const auto fixture=temp.path/"fixture.csv";
    const std::string head="LIBRARY_SNAPSHOT,1\n";
    const std::string b="BOOK,1,A,B,C,1\n";
    const std::string m="MEMBER,1,Name,contact,1\n";
    const std::string l="LOAN,1,1,1,2026-01-01,2026-01-15,\n";
    const std::vector<std::string> bad={
        "", "LIBRARY_SNAPSHOT,2\nEND,0,0,0\n", head+b,
        head+b+"END,0,0,0\n", head+b+b+"END,2,0,0\n",
        head+m+m+"END,0,2,0\n", head+b+m+l+l+"END,1,1,2\n",
        head+l+"END,0,0,1\n", head+b+l+"END,1,0,1\n", head+m+l+"END,0,1,1\n",
        head+"BOOK,1,A,B,C,0\n"+m+l+"END,1,1,1\n",
        head+b+"MEMBER,1,Name,contact,0\n"+l+"END,1,1,1\n",
        head+"BOOK,1,\"unterminated,B,C,1\nEND,1,0,0\n",
        head+"BOOK,1,A\"quote,B,C,1\nEND,1,0,0\n",
        head+"BOOK,1,\"closed\"junk,B,C,1\nEND,1,0,0\n",
        head+"BOOK,1,A,B,C,999999999999999999999\nEND,1,0,0\n",
        head+"BOOK,-1,A,B,C,1\nEND,1,0,0\n",
        head+"MEMBER,1,A,B,2\nEND,0,1,0\n",
        head+"LOAN,1,1,1,2026-02-30,2026-03-10,\nEND,0,0,1\n",
        head+b+m+"LOAN,1,1,1,2026-02-01,2026-01-15,\nEND,1,1,1\n",
        head+b+m+"LOAN,1,1,1,2026-01-01,2026-01-15,2025-12-31\nEND,1,1,1\n",
        head+"BOOK,1,A,B,C\nEND,1,0,0\n", head+"ALIEN,1\nEND,0,0,0\n",
        head+"BOOK,1,"+std::string(513,'a')+",B,C,1\nEND,1,0,0\n",
        head+"END,0,0,0\n\n", head+"END,0,0,0\r", head+"END,0,0,0,1,2,3,4,5\n"
    };
    for(const auto& bytes:bad) {
        writeFile(fixture,bytes);
        rejects([&]{ (void)StorageManager::readSnapshot(fixture); });
    }
    std::string crlf;
    for(char c:original) { if(c=='\n') crlf+='\r'; crlf+=c; }
    writeFile(fixture,crlf);
    CHECK(StorageManager::serialize(StorageManager::readSnapshot(fixture))==original);
    writeFile(fixture,original.substr(0,original.size()-1));
    CHECK(StorageManager::serialize(StorageManager::readSnapshot(fixture))==original);
    // Failures must preserve the old primary; use deterministic filesystem obstacles (also work as admin).
    const auto safe=temp.path/"safe.csv";
    StorageManager::save(lib,safe);
    writeFile(safe.string()+".tmp","interrupted write");
    rejects([&]{ StorageManager::save(LibraryManager{},safe); });
    CHECK(readFile(safe)==current && readFile(safe.string()+".tmp")=="interrupted write");
    std::filesystem::remove(safe.string()+".tmp");
    std::filesystem::create_directory(safe.string()+".bak");
    rejects([&]{ StorageManager::save(LibraryManager{},safe); });
    CHECK(readFile(safe)==current);
    rejects([&]{ StorageManager::save(lib,temp.path); });
    const auto parentFile=temp.path/"not-directory"; writeFile(parentFile,"file");
    rejects([&]{ StorageManager::save(lib,parentFile/"db.csv"); });
    const auto huge=temp.path/"huge.csv";
    { std::ofstream out(huge,std::ios::binary); out.seekp(32*1024*1024); out.put('x'); }
    rejects([&]{ (void)StorageManager::readSnapshot(huge); });
    LibraryManager empty;
    const auto blank=temp.path/"nested"/"empty.csv";
    StorageManager::save(empty,blank);
    CHECK(StorageManager::load(blank).library.loans().empty());
    // The 100001st record is rejected, even with otherwise valid bounded fields/IDs.
    { std::ofstream out(fixture,std::ios::binary); out<<head;
      for(int id=1;id<=100001;++id) out<<"BOOK,"<<id<<",A,B,C,0\n";
      out<<"END,100001,0,0\n"; }
    rejects([&]{ (void)StorageManager::readSnapshot(fixture); });
}); }
