#include "LibraryManager.hpp"
#include "StorageManager.hpp"
#include "Parsing.hpp"
#include <iomanip>
#include <iostream>
#include <optional>
#include <utility>

using namespace library;
namespace {
struct EndInput {};
std::string input(const std::string& prompt) {
    std::cout<<prompt<<std::flush;
    std::string value;
    if (!std::getline(std::cin,value)) throw EndInput{};
    if (!value.empty() && value.back()=='\r') value.pop_back();
    require(value.size()<=512,"Input exceeds 512 bytes.");
    return value;
}
int number(const std::string& prompt, int min=1, int max=maxId) {
    return parseNumber(input(prompt),min,max);
}
std::string cell(std::string value,std::size_t width) {
    for (char& c:value) if (c=='\r' || c=='\n' || c=='\t') c=' ';
    if(value.size()>width) value=value.substr(0,width-3)+"...";
    return value;
}
void books(const LibraryManager& lib, const std::vector<int>& ids) {
    if (ids.empty()) { std::cout<<"No books found.\n"; return; }
    std::cout<<std::left<<std::setw(12)<<"ID"<<std::setw(31)<<"Title"<<std::setw(23)<<"Author"
             <<std::setw(18)<<"Genre"<<"Available/Total\n";
    for (int id:ids) {
        const auto& b=lib.books().at(id);
        std::cout<<std::setw(12)<<id<<std::setw(31)<<cell(b.title(),30)<<std::setw(23)<<cell(b.author(),22)
                 <<std::setw(18)<<cell(b.genre(),17)<<lib.available(id)<<'/'<<b.copies()<<'\n';
    }
}
void loans(const LibraryManager& lib,bool activeOnly) {
    bool any=false;
    for (const auto& [id,l]:lib.loans()) {
        if (activeOnly && !l.active()) continue;
        if(!any) std::cout<<std::left<<std::setw(12)<<"Loan"<<std::setw(12)<<"Book"<<std::setw(12)<<"Member"
            <<std::setw(13)<<"Issued"<<std::setw(13)<<"Due"<<"Returned/State\n";
        any=true;
        std::cout<<std::setw(12)<<id<<std::setw(12)<<l.bookId()<<std::setw(12)<<l.memberId()
                 <<std::setw(13)<<l.issued().str()<<std::setw(13)<<l.due().str()
                 <<(l.active()?"ACTIVE":l.returned()->str())<<'\n';
    }
    if(!any) std::cout<<"No loans found.\n";
}
void menu() {
    std::cout<<"\n==========================================\n"
             <<"        LIBRARY MANAGEMENT SYSTEM\n"
             <<"==========================================\n"
             <<"1. Add Book\n2. View All Books\n3. Search Books\n4. Register Member\n5. View Members\n"
             <<"6. Issue / Borrow Book\n7. Return Book\n8. View Active Loans\n9. View Loan History\n"
             <<"10. Check Book Availability\n11. Import Sample Library Data\n12. Save Library Records\n"
             <<"13. Library Summary\n14. Exit\n==========================================\n";
}
}
int main(int argc,char** argv) {
    std::filesystem::path database="runtime/library.csv", sample="data/sample_library.csv";
    try {
        for(int i=1;i<argc;++i) {
            const std::string arg=argv[i];
            if(arg=="--help") {
                std::cout<<"Usage: library_manager [--db PATH] [--sample PATH]\n"
                         <<"One process per database. Dates use UTC; loans last 14 days.\n"; return 0;
            }
            require((arg=="--db" || arg=="--sample") && i+1<argc,"Expected --db PATH or --sample PATH.");
            if(arg=="--db") database=argv[++i]; else sample=argv[++i];
        }
        // Do not allow the CLI to overwrite the committed import source or its companions.
        const auto normalized=[](const std::filesystem::path& p) { return std::filesystem::weakly_canonical(p); };
        for (const auto& suffix:{"",".bak",".tmp",".stage",".corrupt"}) {
            const std::filesystem::path destination(database.string()+suffix);
            const bool aliases=std::filesystem::exists(destination) && std::filesystem::exists(sample)
                && std::filesystem::equivalent(destination,sample);
            require(!aliases && normalized(destination)!=normalized(sample),
                    "Database paths must be separate from sample source.");
        }
        auto loaded=StorageManager::load(database);
        LibraryManager lib=std::move(loaded.library);
        std::cout<<loaded.notice<<"\nChanges are saved after each successful operation.\n";
        bool running=true;
        while(running) {
            menu();
            try {
                const int choice=number("Choice: ",1,14);
                // Mutations are staged in memory and published only after a successful disk save.
                std::optional<LibraryManager> candidate;
                std::string confirmation;
                switch(choice) {
                case 1: {
                    const int id=number("Book ID: ");
                    const auto title=input("Title: "), author=input("Author: "), genre=input("Genre: ");
                    const int copies=number("Total copies (0-1000000): ",0,maxCopies);
                    candidate=lib; candidate->addBook(Book(id,title,author,genre,copies));
                    confirmation="Book added."; break;
                }
                case 2: {
                    std::vector<int> ids;
                    for(const auto& entry:lib.books()) ids.push_back(entry.first);
                    books(lib,ids); break;
                }
                case 3: {
                    const auto field=input("Search field (title/author/id/genre): ");
                    const auto query=input("Search text: ");
                    books(lib,lib.search(field,query)); break;
                }
                case 4: {
                    const auto name=input("Member name: "), contact=input("Contact (fictional for demos): ");
                    candidate=lib;
                    const int id=candidate->registerMember(name,contact);
                    confirmation="Member registered. ID: "+std::to_string(id); break;
                }
                case 5: {
                    if(lib.members().empty()) std::cout<<"No members registered.\n";
                    else {
                        std::cout<<std::left<<std::setw(12)<<"ID"<<std::setw(27)<<"Name"<<std::setw(31)<<"Contact"
                                 <<std::setw(12)<<"Status"<<"Active loans\n";
                        for(const auto& [id,m]:lib.members())
                            std::cout<<std::setw(12)<<id<<std::setw(27)<<cell(m.name(),26)
                                     <<std::setw(31)<<cell(m.contact(),30)<<std::setw(12)<<(m.active()?"Active":"Inactive")
                                     <<lib.activeForMember(id)<<'\n';
                    } break;
                }
                case 6: {
                    const int book=number("Book ID: "), member=number("Member ID: ");
                    candidate=lib;
                    const int id=candidate->issue(book,member);
                    confirmation="Book issued. Loan ID: "+std::to_string(id)+"; due: "+candidate->loans().at(id).due().str();
                    break;
                }
                case 7: {
                    const int id=number("Loan ID: "); candidate=lib; candidate->returnBook(id);
                    confirmation="Book returned. Loan ID: "+std::to_string(id); break;
                }
                case 8: loans(lib,true); break;
                case 9: loans(lib,false); break;
                case 10: {
                    const int id=number("Book ID: ");
                    const int available=lib.available(id);
                    std::cout<<"Available copies: "<<available<<" / "<<lib.books().at(id).copies()<<'\n'; break;
                }
                case 11: {
                    const auto imported=StorageManager::readSnapshot(sample); candidate=lib;
                    if(candidate->merge(imported)) confirmation="Sample data imported.";
                    else { candidate.reset(); std::cout<<"Sample data already present; no changes.\n"; }
                    break;
                }
                case 12: StorageManager::save(lib,database); std::cout<<"Library records saved.\n"; break;
                case 13: {
                    std::size_t active=0, total=0;
                    for(const auto& p:lib.loans()) if(p.second.active()) ++active;
                    // At most 100000 books * 1000000 copies; use a wide accumulator.
                    unsigned long long copies=0;
                    for(const auto& p:lib.books()) copies+=static_cast<unsigned long long>(p.second.copies());
                    total=lib.loans().size();
                    std::cout<<"Books: "<<lib.books().size()<<" | Members: "<<lib.members().size()
                             <<" | Active loans: "<<active<<" | Returned loans: "<<total-active
                             <<" | Total copies: "<<copies<<'\n'; break;
                }
                case 14: std::cout<<"Goodbye. All successful changes are saved.\n"; running=false; break;
                }
                if(candidate) {
                    StorageManager::save(*candidate,database);
                    lib=std::move(*candidate);
                    std::cout<<confirmation<<" Saved.\n";
                }
            } catch(const EndInput&) {
                std::cout<<"\nEnd of input. All successful changes are saved.\n"; running=false;
            } catch(const std::exception& e) {
                std::cout<<"Error: "<<e.what()<<"\nOperation was not completed.\n";
            }
        }
    } catch(const std::exception& e) {
        std::cerr<<"Startup error: "<<e.what()<<'\n'; return 1;
    }
    return 0;
}
