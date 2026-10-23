#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <cstdint>

// Versioned whole-snapshot envelope. The game protocol's fragmentation layer
// must reassemble this envelope before decode; never feed partial data to PVZ.
namespace snapshot {
constexpr DWORD Magic=0x31535750; // PWS1, little-endian
constexpr DWORD Version=5;       // Raw payload is PWV2, not a native Board/RUN1 bundle
constexpr DWORD MaxRaw=8*1024*1024;
constexpr DWORD MaxPacked=1024*1024;
struct Header {DWORD magic,version,rawBytes,packedBytes,rawCrc,sequence,tick,context,packedCrc,headerCrc;};
static_assert(sizeof(Header)==40,"wire layout");
struct Buffer {BYTE* data;DWORD size;};
inline void release(Buffer& b) {if(b.data) VirtualFree(b.data,0,MEM_RELEASE);b.data=nullptr;b.size=0;}
inline bool allocate(Buffer& b,DWORD size) {
    b.data=static_cast<BYTE*>(VirtualAlloc(nullptr,size,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));
    b.size=b.data?size:0;return b.data!=nullptr;
}
inline DWORD crc32(const void* source,DWORD length) {
    const BYTE* bytes=static_cast<const BYTE*>(source);DWORD crc=0xffffffff;
    for(DWORD i=0;i<length;++i) {
        crc^=bytes[i];
        for(int bit=0;bit<8;++bit) crc=(crc>>1)^(0xedb88320u & (0u-(crc&1)));
    }
    return ~crc;
}
struct Codec {
    using Create=BOOL (WINAPI*)(DWORD,void*,HANDLE*);
    using Transform=BOOL (WINAPI*)(HANDLE,const void*,SIZE_T,void*,SIZE_T,SIZE_T*);
    using Close=BOOL (WINAPI*)(HANDLE);
    HMODULE module=nullptr;
    Create createEncoder=nullptr,createDecoder=nullptr;
    Transform compress=nullptr,decompress=nullptr;
    Close closeEncoder=nullptr,closeDecoder=nullptr;
    Codec()=default;
    Codec(const Codec&)=delete;
    Codec& operator=(const Codec&)=delete;
    ~Codec(){if(module)FreeLibrary(module);}
    bool init() {
        if(module) return createEncoder&&createDecoder&&compress&&decompress&&closeEncoder&&closeDecoder;
        module=LoadLibraryExW(L"cabinet.dll",nullptr,0x800); // System32 only
        if(!module) return false;
        createEncoder=reinterpret_cast<Create>(GetProcAddress(module,"CreateCompressor"));
        createDecoder=reinterpret_cast<Create>(GetProcAddress(module,"CreateDecompressor"));
        compress=reinterpret_cast<Transform>(GetProcAddress(module,"Compress"));
        decompress=reinterpret_cast<Transform>(GetProcAddress(module,"Decompress"));
        closeEncoder=reinterpret_cast<Close>(GetProcAddress(module,"CloseCompressor"));
        closeDecoder=reinterpret_cast<Close>(GetProcAddress(module,"CloseDecompressor"));
        return createEncoder&&createDecoder&&compress&&decompress&&closeEncoder&&closeDecoder;
    }
    bool encode(const BYTE* raw,DWORD bytes,DWORD sequence,DWORD tick,Buffer& out,DWORD context=0) {
        if(out.data || !raw || !bytes || bytes>MaxRaw || !init()) return false;
        HANDLE encoder=nullptr;
        if(!createEncoder(4,nullptr,&encoder)) return false;
        SIZE_T needed=0;
        compress(encoder,raw,bytes,nullptr,0,&needed);
        if(!needed || needed>MaxPacked || !allocate(out,needed+sizeof(Header))) {closeEncoder(encoder);return false;}
        SIZE_T written=0;
        bool ok=compress(encoder,raw,bytes,out.data+sizeof(Header),needed,&written)!=0;
        closeEncoder(encoder);
        if(!ok || !written || written>needed) {release(out);return false;}
        auto h=reinterpret_cast<Header*>(out.data);
        h->magic=Magic;h->version=Version;h->rawBytes=bytes;h->packedBytes=written;
        h->rawCrc=crc32(raw,bytes);h->sequence=sequence;h->tick=tick;
        h->context=context;
        h->packedCrc=crc32(out.data+sizeof(Header),written);
        h->headerCrc=crc32(h,sizeof(Header)-4);
        out.size=written+sizeof(Header);return true;
    }
    bool decode(const BYTE* packet,DWORD bytes,Buffer& out,Header* metadata=nullptr) {
        if(out.data || !packet || bytes<sizeof(Header)) return false;
        // Explicit copy supports unaligned packet storage and avoids aliasing.
        Header h;auto dest=reinterpret_cast<BYTE*>(&h);
        for(DWORD i=0;i<sizeof(Header);++i) dest[i]=packet[i];
        if(h.magic!=Magic || h.version!=Version || h.headerCrc!=crc32(&h,sizeof(Header)-4) ||
           !h.rawBytes || h.rawBytes>MaxRaw || !h.packedBytes || h.packedBytes>MaxPacked ||
           h.packedBytes!=bytes-sizeof(Header) ||
           h.packedCrc!=crc32(packet+sizeof(Header),h.packedBytes) || !init()) return false;
        HANDLE decoder=nullptr;
        if(!createDecoder(4,nullptr,&decoder)) return false;
        if(!allocate(out,h.rawBytes)) {closeDecoder(decoder);return false;}
        SIZE_T written=0;
        bool ok=decompress(decoder,packet+sizeof(Header),h.packedBytes,out.data,h.rawBytes,&written)!=0;
        closeDecoder(decoder);
        if(!ok || written!=h.rawBytes || crc32(out.data,out.size)!=h.rawCrc) {release(out);return false;}
        if(metadata) *metadata=h;
        return true;
    }
};
inline bool readFile(const wchar_t* path,Buffer& out,DWORD limit) {
    if(out.data) return false;
    HANDLE f=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(f==INVALID_HANDLE_VALUE) return false;
    DWORD high=0,size=GetFileSize(f,&high),got=0;
    bool ok=!high && size && size<=limit && allocate(out,size);
    if(ok) ok=ReadFile(f,out.data,size,&got,nullptr) && got==size;
    CloseHandle(f);if(!ok) release(out);return ok;
}
inline bool writeFile(const wchar_t* path,const Buffer& data) {
    HANDLE f=CreateFileW(path,0x40000000,FILE_SHARE_READ,nullptr,2,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(f==INVALID_HANDLE_VALUE) return false;
    DWORD written=0;bool ok=WriteFile(f,data.data,data.size,&written,nullptr) && written==data.size;
    CloseHandle(f);return ok;
}
}
