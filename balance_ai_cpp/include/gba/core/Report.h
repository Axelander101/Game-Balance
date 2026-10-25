#pragma once
#include "gba/core/Model.h"
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>

namespace gba {

class CsvAppender {
public:
    CsvAppender(std::filesystem::path path, std::string header);
    void append(const std::string& row);
private:
    std::filesystem::path path_;
    std::mutex mutex_;
};

std::string csvEscape(const std::string& s);
std::string nowIso();
void ensureRunFiles(const std::filesystem::path& outDir);

} // namespace gba
