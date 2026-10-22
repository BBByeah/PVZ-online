#pragma once
#include "AutomaticRooms.h"
#include "NetSession.h"

namespace RelayNodes {
// Owns only the initial create attempt. Established rooms and rejoining never
// enter this controller, including after the panel is reopened.
class RoomCreateAttempt {
    std::vector<Endpoint> candidates;
    size_t next=0;
    std::chrono::steady_clock::time_point began{};
    bool active=false;
    bool Next(NetSession& session) {
        if(next>=candidates.size()){active=false;return false;}
        const auto endpoint=candidates[next++];began=std::chrono::steady_clock::now();
        session.StartRoomHost(endpoint.host.c_str(),endpoint.port);return true;
    }
public:
    bool Active()const{return active;}
    void Start(NetSession& session,const std::vector<Endpoint>& endpoints) {
        candidates=endpoints;next=0;active=!candidates.empty();if(active)Next(session);
    }
    void Cancel(NetSession& session) {
        if(active&&!session.IsConnected())session.Close();active=false;candidates.clear();
    }
    void Update(NetSession& session) {
        if(!active)return;
        if(session.IsConnected()||session.GetState()==NetSession::State::OFF){active=false;return;}
        if(session.GetState()==NetSession::State::DEAD){if(session.CanRetryRoomCreate())Next(session);else active=false;return;}
        // A connected request with an unknown result cannot be retried on a
        // different node. Before any create bytes were attempted it is safe.
        if(!session.RoomCreateRequestAttempted()&&std::chrono::steady_clock::now()-began>std::chrono::seconds(5)){
            if(next<candidates.size()){session.Close();Next(session);}else{session.FailUnsentRoomCreate();active=false;}
        }
    }
};
}
