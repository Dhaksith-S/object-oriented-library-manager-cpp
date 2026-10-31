#include "StorageManager.hpp"
#include "Parsing.hpp"
#include <fstream>
#include <sstream>
#include <system_error>

namespace library {
namespace {
namespace fs = std::filesystem;
constexpr std::uintmax_t maxFileBytes = 32 * 1024 * 1024;
using Rows = std::vector<std::vector<std::string>>;
Rows parseCsv(const std::string& bytes) {
    Rows rows;
    std::vector<std::string> row;
    std::string field;
    bool quoted = false, closed = false, started = false;
    const auto endField = [&] {
        require(row.size()<8, "Too many CSV columns.");
        row.push_back(field); field.clear(); closed=false; started=false;
    };
    const auto endRow = [&] {
        endField();
        require(rows.size()<3*maxRecords+2, "Too many CSV records.");
        rows.push_back(row); row.clear();
    };
    for (std::size_t i=0; i<bytes.size(); ++i) {
        const char c=bytes[i];
        if (quoted) {
            if (c=='"') {
                if (i+1<bytes.size() && bytes[i+1]=='"') { field+='"'; ++i; }
                else { quoted=false; closed=true; }
            } else field+=c;
        } else if (c==',') endField();
        else if (c=='\n' || c=='\r') {
            if (c=='\r') {
                require(i+1<bytes.size() && bytes[i+1]=='\n', "Bare CR outside quoted CSV field."); ++i;
            }
            endRow();
        } else if (c=='"') {
            require(!started && !closed && field.empty(), "Quote inside unquoted CSV field.");
            quoted=true; started=true;
        } else {
            require(!closed, "Unexpected text after closing CSV quote.");
            field+=c; started=true;
        }
        require(field.size()<=512, "CSV field exceeds 512 bytes.");
    }
    require(!quoted, "Unterminated quoted CSV field.");
    if (started || closed || !field.empty() || !row.empty()) endRow();
    return rows;
}
std::string quote(const std::string& text) {
    std::string out="\"";
    for (char c:text) { out+=c; if(c=='"') out+='"'; }
    return out+'"';
}
void regularOrAbsent(const fs::path& p) {
    require(!fs::exists(p) || fs::is_regular_file(p), "Storage path is not a regular file.");
}
void writeChecked(const fs::path& path, const std::string& text) {
    std::ofstream out(path,std::ios::binary|std::ios::trunc);
    if (!out) throw std::runtime_error("Cannot open staged snapshot for writing.");
    out.write(text.data(),static_cast<std::streamsize>(text.size()));
    out.flush();
    if (!out) throw std::runtime_error("Snapshot write/flush failed.");
    out.close();
    if (!out) throw std::runtime_error("Snapshot close failed.");
}
}
LibraryManager StorageManager::readSnapshot(const std::filesystem::path& path) {
    require(std::filesystem::is_regular_file(path), "Snapshot file does not exist or is not a regular file.");
    const auto size=std::filesystem::file_size(path);
    require(size<=maxFileBytes, "Snapshot exceeds 32 MiB limit.");
    std::ifstream in(path,std::ios::binary);
    if (!in) throw std::runtime_error("Cannot open snapshot.");
    std::string bytes(static_cast<std::size_t>(size),'\0');
    in.read(bytes.data(),static_cast<std::streamsize>(size));
    require(in.gcount()==static_cast<std::streamsize>(size) && !in.bad(), "Snapshot read failed.");
    require(in.peek()==std::char_traits<char>::eof(), "Snapshot changed during read.");
    const auto rows=parseCsv(bytes);
    require(rows.size()>=2 && rows.front()==std::vector<std::string>{"LIBRARY_SNAPSHOT","1"},
            "Missing or unsupported snapshot version.");
    std::vector<Book> books;
    std::vector<Member> members;
    std::vector<Loan> loans;
    for (std::size_t i=1;i+1<rows.size();++i) {
        const auto& r=rows[i];
        try {
            if (r[0]=="BOOK") {
                require(r.size()==6, "BOOK requires 6 columns.");
                require(books.size()<maxRecords,"Book record limit exceeded.");
                books.emplace_back(parseNumber(r[1],1,maxId),r[2],r[3],r[4],parseNumber(r[5],0,maxCopies));
            } else if (r[0]=="MEMBER") {
                require(r.size()==5, "MEMBER requires 5 columns.");
                require(members.size()<maxRecords,"Member record limit exceeded.");
                members.emplace_back(parseNumber(r[1],1,maxId),r[2],r[3],parseNumber(r[4],0,1)==1);
            } else if (r[0]=="LOAN") {
                require(r.size()==7, "LOAN requires 7 columns.");
                require(loans.size()<maxRecords,"Loan record limit exceeded.");
                std::optional<Date> returned;
                if (!r[6].empty()) returned=Date::parse(r[6]);
                loans.emplace_back(parseNumber(r[1],1,maxId),parseNumber(r[2],1,maxId),
                    parseNumber(r[3],1,maxId),Date::parse(r[4]),Date::parse(r[5]),returned);
            } else throw std::invalid_argument("Unknown record type.");
        } catch (const std::exception& e) {
            throw std::invalid_argument("CSV record "+std::to_string(i+1)+": "+e.what());
        }
    }
    const auto& footer=rows.back();
    require(footer.size()==4 && footer[0]=="END", "Missing snapshot END/count footer (possibly truncated).");
    require(static_cast<std::size_t>(parseNumber(footer[1],0,static_cast<int>(maxRecords)))==books.size()
         && static_cast<std::size_t>(parseNumber(footer[2],0,static_cast<int>(maxRecords)))==members.size()
         && static_cast<std::size_t>(parseNumber(footer[3],0,static_cast<int>(maxRecords)))==loans.size(),
            "Snapshot footer counts do not match records.");
    return LibraryManager::fromRecords(std::move(books),std::move(members),std::move(loans));
}
std::string StorageManager::serialize(const LibraryManager& library) {
    library.validate();
    std::ostringstream out;
    out<<"LIBRARY_SNAPSHOT,1\n";
    for (const auto& [id,b]:library.books())
        out<<"BOOK,"<<id<<','<<quote(b.title())<<','<<quote(b.author())<<','<<quote(b.genre())<<','<<b.copies()<<'\n';
    for (const auto& [id,m]:library.members())
        out<<"MEMBER,"<<id<<','<<quote(m.name())<<','<<quote(m.contact())<<','<<(m.active()?1:0)<<'\n';
    for (const auto& [id,l]:library.loans())
        out<<"LOAN,"<<id<<','<<l.bookId()<<','<<l.memberId()<<','<<l.issued().str()<<','<<l.due().str()
           <<','<<(l.returned()?l.returned()->str():"")<<'\n';
    out<<"END,"<<library.books().size()<<','<<library.members().size()<<','<<library.loans().size()<<'\n';
    auto result=out.str();
    require(result.size()<=maxFileBytes, "Serialized snapshot exceeds 32 MiB limit.");
    return result;
}
LoadResult StorageManager::load(const std::filesystem::path& path) {
    const auto backup=std::filesystem::path(path.string()+".bak");
    regularOrAbsent(path); regularOrAbsent(backup);
    if (!std::filesystem::exists(path) && !std::filesystem::exists(backup))
        return {LibraryManager{},"No saved records. Starting an empty library."};
    try { return {readSnapshot(path),"Saved records loaded."}; }
    catch (const std::exception& primary) {
        const std::string reason=primary.what();
        try { return {readSnapshot(backup),"WARNING: recovered previous backup; primary unavailable/invalid: "+reason}; }
        catch (const std::exception& fallback) {
            throw std::runtime_error("Cannot load primary ("+reason+") or backup ("+fallback.what()
                                     +"). Existing files were preserved.");
        }
    }
}
void StorageManager::save(const LibraryManager& library, const std::filesystem::path& path) {
    namespace fs=std::filesystem;
    const auto bytes=serialize(library);
    const fs::path backup(path.string()+".bak"), staged(path.string()+".tmp"), backupStage(path.string()+".stage");
    regularOrAbsent(path); regularOrAbsent(backup);
    require(!fs::exists(staged) && !fs::exists(backupStage),
            "Stale .tmp/.stage file found. Inspect it and move it aside before saving.");
    if (!path.parent_path().empty()) fs::create_directories(path.parent_path());
    bool madeStage=false, madeBackupStage=false;
    try {
        madeStage=true; writeChecked(staged,bytes);
        (void)readSnapshot(staged); // Verify staged bytes before touching an existing snapshot.
        bool primaryValid=false;
        if (fs::exists(path)) {
            try { (void)readSnapshot(path); primaryValid=true; }
            catch (const std::exception&) {
                // Keep the corrupt primary for diagnosis; never overwrite a good recovery backup.
                require(!fs::exists(path.string()+".corrupt"), "Existing .corrupt file must be moved aside first.");
                if (fs::exists(backup)) (void)readSnapshot(backup);
                fs::copy_file(path,path.string()+".corrupt");
            }
        }
        if (primaryValid) {
            madeBackupStage=true;
            fs::copy_file(path,backupStage);
            (void)readSnapshot(backupStage);
            if (fs::exists(backup)) fs::remove(backup);
            fs::rename(backupStage,backup); madeBackupStage=false;
        }
        // Portable C++17 cannot guarantee atomic replacement or fsync durability.
        if (fs::exists(path)) fs::remove(path);
        fs::rename(staged,path); madeStage=false;
    } catch (...) {
        std::error_code ignored;
        if (madeStage) fs::remove(staged,ignored);
        if (madeBackupStage) fs::remove(backupStage,ignored);
        throw;
    }
}
}
