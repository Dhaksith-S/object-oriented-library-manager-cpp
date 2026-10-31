#pragma once
#include "LibraryManager.hpp"
#include <filesystem>

namespace library {
struct LoadResult {
    LibraryManager library;
    std::string notice;
};
class StorageManager {
public:
    static LibraryManager readSnapshot(const std::filesystem::path& path);
    static LoadResult load(const std::filesystem::path& path);
    static void save(const LibraryManager& library, const std::filesystem::path& path);
    static std::string serialize(const LibraryManager& library);
};
}
