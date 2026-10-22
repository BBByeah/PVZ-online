#pragma once
#include <string>
#include <cstdint>
#include <windows.h>
#include "../ModText.h"

// Own only the helper launched by this game. The job also cleans it up if the
// game crashes; returning to the in-game menu does not destroy this controller.
namespace LocalHostControl {
struct Owner {
    HANDLE process=nullptr, job=nullptr;
    std::wstring ready;
    uint16_t requested=0;
    ~Owner(){Stop();}
    void Stop() {
        if(process){
            if(WaitForSingleObject(process,0)==WAIT_TIMEOUT){
                HANDLE f=CreateFileW(ready.c_str(),GENERIC_WRITE,FILE_SHARE_READ,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
                if(f!=INVALID_HANDLE_VALUE){DWORD n;WriteFile(f,"stop",4,&n,nullptr);CloseHandle(f);}
                if(WaitForSingleObject(process,1500)==WAIT_TIMEOUT)TerminateProcess(process,0);
            }
            WaitForSingleObject(process,3000);CloseHandle(process);process=nullptr;
        }
        if(job){CloseHandle(job);job=nullptr;}
        if(!ready.empty())DeleteFileW(ready.c_str());ready.clear();requested=0;
    }
};
inline Owner& State(){static Owner owner;return owner;}
inline bool Alive(){auto& s=State();return s.process&&WaitForSingleObject(s.process,0)==WAIT_TIMEOUT;}
inline uint16_t Port(){
    auto& s=State();if(!Alive())return 0;
    HANDLE f=CreateFileW(s.ready.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(f==INVALID_HANDLE_VALUE)return 0;
    char bytes[24]{};DWORD n=0;ReadFile(f,bytes,23,&n,nullptr);CloseHandle(f);
    return std::string(bytes,n)==std::to_string(s.requested)?s.requested:0;
}
inline bool Starting(){return Alive()&&!Port();}
inline std::string Error(){return ModText::Tr("无法开放联机端口，请检查端口是否被占用后重试。","Could not open the hosting port. Check that it is free and retry.");}
inline void Stop(){State().Stop();}
inline bool Copy(const std::string& text){
    if(!OpenClipboard(nullptr))return false;
    HGLOBAL bytes=GlobalAlloc(GMEM_MOVEABLE,text.size()+1);if(!bytes){CloseClipboard();return false;}
    void* memory=GlobalLock(bytes);if(!memory){GlobalFree(bytes);CloseClipboard();return false;}
    memcpy(memory,text.c_str(),text.size()+1);GlobalUnlock(bytes);EmptyClipboard();
    const bool ok=SetClipboardData(CF_TEXT,bytes)!=nullptr;if(!ok)GlobalFree(bytes);CloseClipboard();return ok;
}
inline bool Start(uint16_t port,std::string& error) {
    auto& s=State();if(Alive()){if(s.requested==port)return true;error=ModText::Tr("请先结束当前房间，再更换端口。","End the current host before changing its port.");return false;}
    s.Stop();
    wchar_t executable[MAX_PATH],directory[MAX_PATH],temp[MAX_PATH],file[MAX_PATH];
    if(!GetModuleFileNameW(nullptr,executable,MAX_PATH)||!GetTempPathW(MAX_PATH,temp)||!GetTempFileNameW(temp,L"pvz",0,file)){error=ModText::Tr("无法准备本机房间。","Cannot prepare local hosting.");return false;}
    std::wstring path=executable;auto slash=path.find_last_of(L"\\/");path.resize(slash+1);
    std::wstring helper=path+L"pvz-lan-host.exe";
    s.ready=file;s.requested=port;
    std::wstring command=L"\""+helper+L"\" -private -port "+std::to_wstring(port)+L" -ready-file \""+s.ready+L"\"";
    s.job=CreateJobObjectW(nullptr,nullptr);
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};limits.BasicLimitInformation.LimitFlags=JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    if(!s.job||!SetInformationJobObject(s.job,JobObjectExtendedLimitInformation,&limits,sizeof(limits))){s.Stop();error=ModText::Tr("无法创建房间进程组。","Cannot create the host process group.");return false;}
    STARTUPINFOW si{};si.cb=sizeof(si);si.dwFlags=STARTF_USESHOWWINDOW;si.wShowWindow=SW_HIDE;
    PROCESS_INFORMATION pi{};
    if(!CreateProcessW(helper.c_str(),&command[0],nullptr,nullptr,FALSE,CREATE_NO_WINDOW|CREATE_SUSPENDED,nullptr,path.c_str(),&si,&pi)){s.Stop();error=ModText::Tr("游戏包缺少开房程序 pvz-lan-host.exe，请用完整包解压后重试。","The complete game package is missing its LAN host helper.");return false;}
    s.process=pi.hProcess;
    if(!AssignProcessToJobObject(s.job,s.process)){TerminateProcess(s.process,1);CloseHandle(pi.hThread);s.Stop();error=ModText::Tr("无法管理房间进程。","Cannot manage the host process.");return false;}
    ResumeThread(pi.hThread);CloseHandle(pi.hThread);return true;
}
}
