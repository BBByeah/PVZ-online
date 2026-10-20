#pragma once
#include "RelayNodes.h"
#include <deque>
#include <memory>
#include <set>
#include <random>

namespace RelayNodes {
// Discovery is independent of the live session. Once connected, a room never migrates.
class Allocator {
    struct Candidate {Node node;std::unique_ptr<Probe> probe;};
    std::vector<Node> queue;
    std::vector<Candidate> probes;
    CatalogDiscovery discovery;
    std::chrono::steady_clock::time_point began{},firstGood{};
    size_t next=0;
    bool discovering=false;
    static std::mt19937& Random(){static std::mt19937 engine(unsigned(std::chrono::steady_clock::now().time_since_epoch().count()));return engine;}
    bool Eligible(const Candidate& candidate)const {
        const auto& p=*candidate.probe;
        if(p.state==Probe::LEGACY)return true;
        return p.state==Probe::READY&&p.capacity.rooms<p.capacity.roomLimit&&p.capacity.connections<p.capacity.connectionLimit&&
            (!p.capacity.detailed||(p.capacity.acceptNew&&p.capacity.sampleAgeMs<=5000));
    }
    double Score(const Candidate& candidate)const {
        const auto& p=*candidate.probe;
        double score=p.state==Probe::LEGACY?1.25:std::max(double(p.capacity.rooms)/p.capacity.roomLimit,double(p.capacity.connections)/p.capacity.connectionLimit);
        if(p.state==Probe::READY&&p.capacity.detailed)score=std::max(score,p.capacity.score);
        const double weight=std::max(1.0,double(candidate.node.weight)*(p.capacity.detailed?p.capacity.weight:100)/100.0);
        return score*100/weight+std::min(p.latencyMs,5000)*0.000001;
    }
    void Begin(const std::vector<Node>& nodes) {
        queue.clear();probes.clear();next=0;firstGood={};began=std::chrono::steady_clock::now();
        for(const auto& n:nodes)if(n.create)queue.push_back(n);
        std::shuffle(queue.begin(),queue.end(),Random());
    }
    void Finish() {
        std::vector<std::pair<double,Endpoint>> ranked;
        for(const auto& candidate:probes)if(Eligible(candidate))ranked.push_back({Score(candidate),candidate.node.endpoint});
        std::shuffle(ranked.begin(),ranked.end(),Random());
        std::stable_sort(ranked.begin(),ranked.end(),[](const auto& a,const auto& b){return a.first<b.first;});
        // Shuffle a narrow band of equivalent loads. Unequal capacities are
        // already represented by the normalized score and configured weight.
        if(!ranked.empty()){size_t count=1;while(count<ranked.size()&&ranked[count].first<=ranked[0].first+0.05)++count;std::shuffle(ranked.begin(),ranked.begin()+count,Random());}
        candidates.clear();for(const auto& v:ranked)candidates.push_back(v.second);
        state=candidates.empty()?UNAVAILABLE:READY;if(state==READY)chosen=candidates.front();probes.clear();
    }
public:
    enum State {IDLE,LOADING,READY,UNAVAILABLE} state=IDLE;
    Endpoint chosen;
    std::vector<Endpoint> candidates;
    void Cancel(){discovery.Close();probes.clear();queue.clear();candidates.clear();discovering=false;state=IDLE;}
    bool Start() {
        if(state==LOADING)return false;Cancel();chosen={};state=LOADING;
        discovering=discovery.Start();if(!discovering)Begin(Catalog());return true;
    }
    bool Start(const std::vector<Node>& nodes) {
        if(state==LOADING)return false;Cancel();chosen={};state=LOADING;Begin(nodes);return true;
    }
    void Update() {
        if(state!=LOADING)return;
        if(discovering){discovery.Update();if(discovery.loading)return;discovering=false;Begin(Catalog());}
        int active=0;for(auto& p:probes){p.probe->Update();if(p.probe->state==Probe::LOADING)++active;}
        while(active<4&&next<queue.size()) {Candidate c{queue[next++],std::make_unique<Probe>()};c.probe->Refresh(c.node.endpoint);probes.push_back(std::move(c));++active;}
        const auto now=std::chrono::steady_clock::now();
        if(firstGood==std::chrono::steady_clock::time_point{})for(const auto& c:probes)if(Eligible(c)){firstGood=now;break;}
        if((active==0&&next==queue.size())||now-began>std::chrono::seconds(5)||
            (firstGood!=std::chrono::steady_clock::time_point{}&&now-firstGood>std::chrono::milliseconds(400)))Finish();
    }
};

struct LocatedRoom {RoomDirectory::Entry room;Endpoint endpoint;};
class Directory {
    struct Source {
        Endpoint endpoint;
        RoomDirectory::Client client;
        std::deque<LocatedRoom> pending;
        int offset=0;
        bool exhausted=false,fetching=false,wanted=false;
    };
    std::vector<std::unique_ptr<Source>> sources;
    std::vector<std::vector<LocatedRoom>> history;
    std::set<std::string> seen;
    std::chrono::steady_clock::time_point last{};
    size_t turn=0;
    CatalogDiscovery discovery;
    bool discovering=false;
    std::chrono::steady_clock::time_point pageBegan{};
    bool anySuccess=false;
    void Pump() {
        size_t active=0;for(const auto& s:sources)if(s->fetching)++active;
        for(auto& s:sources)if(active<4&&s->wanted&&!s->fetching){
            s->fetching=s->client.Refresh(s->endpoint.host,s->offset,s->endpoint.port);
            if(s->fetching)++active;else{s->wanted=false;s->exhausted=true;partial=true;}
        }
    }
    void Fetch() {
        state=LOADING;entries.clear();last=pageBegan=std::chrono::steady_clock::now();
        for(auto& s:sources)s->wanted=s->pending.empty()&&!s->exhausted;
        Pump();
    }
    void Begin(const std::vector<Node>& nodes) {
        for(const auto& node:nodes)if(node.directory){auto source=std::make_unique<Source>();source->endpoint=node.endpoint;sources.push_back(std::move(source));}
        // A large catalog may exceed one bounded refresh when nodes are offline.
        // Rotate the first batch so later nodes get a turn on subsequent refreshes.
        static size_t rotation=0;
        if(!sources.empty()){const size_t offset=rotation%sources.size();std::rotate(sources.begin(),sources.begin()+offset,sources.end());rotation+=4;}
        Fetch();
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
    void Close(){discovery.Close();discovering=false;sources.clear();if(state==LOADING)state=IDLE;}
    bool Refresh() {
        if(!CanRefresh())return false;Close();history.clear();seen.clear();entries.clear();page=0;turn=0;partial=anySuccess=false;
        discovering=discovery.Start();if(discovering){state=LOADING;last=std::chrono::steady_clock::now();}else Begin(Catalog());return true;
    }
    bool Refresh(const std::vector<Node>& nodes) {
        if(!CanRefresh())return false;Close();history.clear();seen.clear();entries.clear();page=0;turn=0;partial=anySuccess=false;Begin(nodes);return true;
    }
    bool Prev(){if(!CanRefresh()||page<=0)return false;entries=history[--page];return true;}
    bool Next(){
        if(!CanRefresh()||!HasNext())return false;
        if(page+1<int(history.size())){entries=history[++page];return true;}
        ++page;Fetch();return true;
    }
    void Update() {
        if(state!=LOADING)return;
        if(discovering){discovery.Update();if(discovery.loading)return;discovering=false;Begin(Catalog());}
        bool waiting=false;
        for(auto& s:sources)if(s->fetching){
            s->client.Update();
            if(s->client.state==RoomDirectory::Client::LOADING){waiting=true;continue;}
            s->fetching=false;s->wanted=false;
            if(s->client.state==RoomDirectory::Client::READY){
                anySuccess=true;
                for(const auto& e:s->client.entries)if(seen.insert(s->endpoint.Text()+"/"+e.code).second)s->pending.push_back({e,s->endpoint});
                s->offset=s->client.next;s->exhausted=s->offset<0;
            }else{s->exhausted=true;partial=true;}
        }
        Pump();waiting=false;
        for(auto& s:sources)if(s->fetching||s->wanted){
            if(std::chrono::steady_clock::now()-pageBegan>std::chrono::seconds(5)){s->client.Close();s->fetching=s->wanted=false;s->exhausted=true;partial=true;}else waiting=true;
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
