#pragma once
#include "RelayNodes.h"
#include <deque>
#include <memory>
#include <set>

namespace RelayNodes {
// Discovery is independent of the live session. Once connected, a room never migrates.
class Allocator {
    std::vector<Endpoint> endpoints;
    std::vector<std::unique_ptr<Probe>> probes;
public:
    enum State {IDLE,LOADING,READY,UNAVAILABLE} state=IDLE;
    Endpoint chosen;
    void Cancel(){probes.clear();state=IDLE;}
    bool Start(const std::vector<Node>& nodes=Catalog()) {
        if(state==LOADING)return false;
        Cancel();endpoints.clear();chosen={};state=LOADING;
        for(const auto& node:nodes){endpoints.push_back(node.endpoint);auto p=std::make_unique<Probe>();p->Refresh(node.endpoint);probes.push_back(std::move(p));}
        return true;
    }
    void Update() {
        if(state!=LOADING)return;
        bool pending=false;for(auto& p:probes){p->Update();pending|=p->state==Probe::LOADING;}
        if(pending)return;
        double best=2;int index=-1;
        for(size_t i=0;i<probes.size();++i){const auto& p=*probes[i];double score;
            if(p.state==Probe::READY){const auto& c=p.capacity;
                if(c.rooms>=c.roomLimit||c.connections>=c.connectionLimit-1)continue;
                score=std::max(double(c.rooms)/c.roomLimit,double(c.connections)/c.connectionLimit);
            }else if(p.state==Probe::LEGACY)score=0.95;else continue;
            // Capacity dominates latency. Unknown legacy load is deliberately conservative.
            score+=std::min(p.latencyMs,5000)*0.000001;
            if(score<best){best=score;index=int(i);}
        }
        if(index>=0){chosen=endpoints[index];state=READY;}else state=UNAVAILABLE;
        probes.clear();
    }
};

struct LocatedRoom {RoomDirectory::Entry room;Endpoint endpoint;};
class Directory {
    struct Source {
        Endpoint endpoint;
        RoomDirectory::Client client;
        std::deque<LocatedRoom> pending;
        int offset=0;
        bool exhausted=false,fetching=false;
    };
    std::vector<std::unique_ptr<Source>> sources;
    std::vector<std::vector<LocatedRoom>> history;
    std::set<std::string> seen;
    std::chrono::steady_clock::time_point last{};
    size_t turn=0;
    bool anySuccess=false;
    void Fetch() {
        state=LOADING;entries.clear();last=std::chrono::steady_clock::now();
        for(auto& s:sources)if(s->pending.empty()&&!s->exhausted){
            s->fetching=s->client.Refresh(s->endpoint.host,s->offset,s->endpoint.port);
            if(!s->fetching){s->exhausted=true;partial=true;}
        }
    }
public:
    enum State {IDLE,LOADING,READY,UNAVAILABLE} state=IDLE;
    std::vector<LocatedRoom> entries;
    int page=0;
    bool partial=false;
    bool CanRefresh()const{return state!=LOADING&&std::chrono::steady_clock::now()-last>=std::chrono::seconds(2);}
    bool HasNext()const {
        if(page+1<int(history.size()))return true;
        for(const auto& s:sources)if(!s->pending.empty()||!s->exhausted)return true;
        return false;
    }
    void Close(){sources.clear();if(state==LOADING)state=IDLE;}
    bool Refresh(const std::vector<Node>& nodes=Catalog()) {
        if(!CanRefresh())return false;
        Close();history.clear();seen.clear();entries.clear();page=0;turn=0;partial=anySuccess=false;
        for(const auto& n:nodes){auto s=std::make_unique<Source>();s->endpoint=n.endpoint;sources.push_back(std::move(s));}
        Fetch();return true;
    }
    bool Prev(){if(!CanRefresh()||page<=0)return false;entries=history[--page];return true;}
    bool Next(){
        if(!CanRefresh()||!HasNext())return false;
        if(page+1<int(history.size())){entries=history[++page];return true;}
        ++page;Fetch();return true;
    }
    void Update() {
        if(state!=LOADING)return;
        bool waiting=false;
        for(auto& s:sources)if(s->fetching){
            s->client.Update();
            if(s->client.state==RoomDirectory::Client::LOADING){waiting=true;continue;}
            s->fetching=false;
            if(s->client.state==RoomDirectory::Client::READY){
                anySuccess=true;
                for(const auto& e:s->client.entries)if(seen.insert(s->endpoint.Text()+"/"+e.code).second)s->pending.push_back({e,s->endpoint});
                s->offset=s->client.next;s->exhausted=s->offset<0;
            }else{s->exhausted=true;partial=true;}
        }
        if(waiting)return;
        // Interleave nodes while retaining each entry's exact destination, even for equal raw codes.
        for(int row=0;row<RoomDirectory::PAGE_SIZE;++row){
            bool found=false;
            for(size_t i=0;i<sources.size();++i){auto& s=*sources[turn++%sources.size()];
                if(s.pending.empty())continue;
                entries.push_back(s.pending.front());s.pending.pop_front();found=true;break;
            }
            if(!found)break;
        }
        history.push_back(entries);state=anySuccess?READY:UNAVAILABLE;
    }
};
}
