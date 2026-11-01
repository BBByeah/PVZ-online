#include "Diagnostics.h"
#include <windows.h>
#include <filesystem>
#include <mutex>
#include <cstdio>
#include <vector>
#include <algorithm>
#include "../../third_party/nlohmann/json.hpp"
namespace TestDiagnostics {
namespace {
std::mutex mutex;std::filesystem::path directory;FILE* file=nullptr;uint64_t bytes=0;unsigned part=0;std::string session;
void Open(){
 auto p=directory/(session+"-"+std::to_string(part++)+".jsonl");file=_wfopen(p.c_str(),L"ab");bytes=0;
 std::vector<std::filesystem::directory_entry> files;std::error_code ec;
 for(auto& f:std::filesystem::directory_iterator(directory,ec))if(f.path().extension()==".jsonl")files.push_back(f);
 std::sort(files.begin(),files.end(),[](auto&a,auto&b){return a.last_write_time()<b.last_write_time();});
 while(files.size()>10){std::filesystem::remove(files.front().path(),ec);files.erase(files.begin());}
}
}
void Initialize(const std::string& instance){
 std::lock_guard lock(mutex);if(file)return;wchar_t exe[32768]{};GetModuleFileNameW(nullptr,exe,32768);
 directory=std::filesystem::path(exe).parent_path()/"logs"/(instance.empty()?"player":instance);
 std::error_code ec;std::filesystem::create_directories(directory,ec);SYSTEMTIME t;GetLocalTime(&t);char id[100];
 sprintf_s(id,"%04u%02u%02u-%02u%02u%02u-pid%lu",t.wYear,t.wMonth,t.wDay,t.wHour,t.wMinute,t.wSecond,GetCurrentProcessId());session=id;Open();
}
void Line(const std::string& event,const std::string& message){
 std::lock_guard lock(mutex);if(!file)return;SYSTEMTIME t;GetLocalTime(&t);char stamp[40];
 sprintf_s(stamp,"%04u-%02u-%02uT%02u:%02u:%02u.%03u",t.wYear,t.wMonth,t.wDay,t.wHour,t.wMinute,t.wSecond,t.wMilliseconds);
 std::string line=nlohmann::json({{"time",stamp},{"pid",GetCurrentProcessId()},{"event",event},{"message",message}}).dump(-1,' ',false,nlohmann::json::error_handler_t::replace)+"\n";
 if(bytes+line.size()>10*1024*1024){fclose(file);file=nullptr;Open();}if(!file)return;
 bytes+=fwrite(line.data(),1,line.size(),file);fflush(file);
}
void Close(){Line("shutdown","normal shutdown");std::lock_guard lock(mutex);if(file)fclose(file);file=nullptr;}
}
