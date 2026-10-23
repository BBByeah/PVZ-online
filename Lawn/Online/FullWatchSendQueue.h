#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <cstdint>
namespace transport {
// Control messages stay ordered, ahead of wholly unsent snapshot frames.
// An unfinished TCP frame always finishes first, regardless of priority.
struct Queue {
    static constexpr UINT Capacity=256,MaxFrame=270;
    struct Frame {UINT size,stream;DWORD generation;BYTE data[MaxFrame];};
    Frame frames[Capacity];
    UINT head=0,count=0,offset=0;
    void reset(){head=count=offset=0;}
    Frame& at(UINT index){return frames[(head+index)%Capacity];}
    void copyFrame(UINT target,UINT source) {
        Frame& a=at(target);Frame& b=at(source);
        a.size=b.size;a.stream=b.stream;a.generation=b.generation;
        for(UINT i=0;i<b.size;++i)a.data[i]=b.data[i];
    }
    UINT discardOlder(UINT stream,DWORD generation) {
        UINT keep=0,removed=0;
        for(UINT i=0;i<count;++i) {
            Frame& frame=at(i);
            bool older=frame.stream==stream && static_cast<LONG>(generation-frame.generation)>0;
            if(stream && older && !(i==0 && offset)){++removed;continue;}
            if(keep!=i)copyFrame(keep,i);
            ++keep;
        }
        count=keep;return removed;
    }
    bool push(const BYTE* data,UINT size,UINT stream=0,DWORD generation=0) {
        if(!data || !size || size>MaxFrame || count==Capacity)return false;
        UINT position=count;
        if(!stream) {
            position=offset?1:0;
            for(UINT i=position;i<count;++i)if(!at(i).stream)position=i+1;
            for(UINT i=count;i>position;--i)copyFrame(i,i-1);
        }
        Frame& frame=at(position);frame.size=size;frame.stream=stream;frame.generation=generation;
        for(UINT i=0;i<size;++i)frame.data[i]=data[i];
        ++count;return true;
    }
    // callback: positive byte count, zero would-block, negative fatal failure.
    template<class Send> int pump(Send send,UINT budget) {
        int progressed=0;
        while(count && budget--) {
            Frame& frame=frames[head];UINT remaining=frame.size-offset;
            int sent=send(frame.data+offset,remaining);
            if(sent<0 || static_cast<UINT>(sent)>remaining)return -1;
            if(!sent)break;
            offset+=sent;progressed+=sent;
            if(offset==frame.size){head=(head+1)%Capacity;--count;offset=0;}
        }
        return progressed;
    }
};
}
