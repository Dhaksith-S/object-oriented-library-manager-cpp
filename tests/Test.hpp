#pragma once
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

inline int checks = 0;
inline void check(bool condition, const char* expression, int line) {
    ++checks;
    if(!condition) throw std::runtime_error(std::string("Check failed at line ")+std::to_string(line)+": "+expression);
}
#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)
template<class F> void rejects(F function) {
    bool caught=false;
    try { function(); } catch(const std::exception&) { caught=true; }
    CHECK(caught);
}
struct TempDirectory {
    std::filesystem::path path;
    TempDirectory() {
        path=std::filesystem::temp_directory_path()/
            ("library-tests-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        if(!std::filesystem::create_directory(path)) throw std::runtime_error("Cannot create isolated test directory.");
    }
    ~TempDirectory() { std::error_code ignored; std::filesystem::remove_all(path,ignored); }
};
inline void writeFile(const std::filesystem::path& path, const std::string& bytes) {
    std::ofstream out(path,std::ios::binary); out<<bytes;
    if(!out) throw std::runtime_error("Test fixture write failed.");
}
inline std::string readFile(const std::filesystem::path& path) {
    std::ifstream in(path,std::ios::binary);
    return {std::istreambuf_iterator<char>(in),std::istreambuf_iterator<char>()};
}
template<class F> int runTests(F tests) {
    try { tests(); std::cout<<checks<<" checks passed.\n"; return 0; }
    catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
