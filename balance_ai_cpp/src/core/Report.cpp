#include "gba/core/Report.h"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace gba {
CsvAppender::CsvAppender(std::filesystem::path path, std::string header):path_(std::move(path)){
    std::filesystem::create_directories(path_.parent_path());
    if(!std::filesystem::exists(path_) || std::filesystem::file_size(path_)==0){std::ofstream f(path_); f<<header<<'\n';}
}
void CsvAppender::append(const std::string& row){std::lock_guard<std::mutex>g(mutex_);std::ofstream f(path_,std::ios::app);f<<row<<'\n';}
std::string csvEscape(const std::string& s){std::string o="\"";for(char c:s){if(c=='\"')o+="\"\"";else o+=c;}o+='\"';return o;}
std::string nowIso(){auto t=std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());std::tm tm{};
#ifdef _WIN32
localtime_s(&tm,&t);
#else
localtime_r(&t,&tm);
#endif
std::ostringstream s;s<<std::put_time(&tm,"%Y-%m-%dT%H:%M:%S");return s.str();}
void ensureRunFiles(const std::filesystem::path& outDir){std::filesystem::create_directories(outDir);}
}
