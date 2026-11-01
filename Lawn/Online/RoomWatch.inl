// Room lifetime UDP channels, independent of PWV2 subscriptions. Tokens come
// exclusively from authenticated room signaling, never from UDP strangers.
bool NetSession::RequestFullWatch(bool relay){
 if(!IsWatching()||!mFullWatch->subscription)return false;
 WatchWire::Bytes p{mLocalSeat,mWatchTargetSeat,0x85};WatchWire::Put(p,mFullWatch->subscription);p.push_back(relay?1:0);
 TodLog("[watch] request_send target=%u subscription=%u relay=%d",mWatchTargetSeat,mFullWatch->subscription,relay);
 return Dispatch(NetProto::MSG_BOARD_WATCH,p.data(),int(p.size()),mWatchTargetSeat);
}
void NetSession::TickRoomDirect(uint64_t now){
 if(!IsConnected())return;
 if(IsWatching()&&(!IsSeatOccupied(mWatchTargetSeat)||IsSeatOffline(mWatchTargetSeat))){EndWatch(false);}
 if(IsWatching()&&!mFullWatch->pictureAt&&!mFullWatch->fallbackRequested&&now-mFullWatch->requestedAt>=2000){
  mFullWatch->fallbackRequested=true;RequestFullWatch(true);
 }
 if(!mObserverP2PEnabled)return;
 bool discovering=false;
 for(uint8_t seat=1;seat<=6;++seat){
  auto&p=mFullWatch->peers[seat];bool present=seat!=mLocalSeat&&IsSeatOccupied(seat)&&!IsSeatOffline(seat);
  if(p.present!=present||(present&&p.identity!=mCoopPeers[seat])){
   if(p.identity&&p.identity!=mCoopPeers[seat]&&IsWatching()&&mWatchTargetSeat==seat)EndWatch(false);
   mPunchLink.DropRoomPeer(seat);p={};p.present=present;p.identity=mCoopPeers[seat];p.started=now;
   mFullWatch->viewerSubscription[seat]=0;mFullWatch->ClearSend(seat);
   if(present)TodLog("[room-p2p] begin peer=%u",seat);
  }
  if(!present)continue;
  bool ready=mPunchLink.RoomReady(seat);
  if(ready&&!p.ready){p.ready=true;p.failed=false;p.renew=now;TodLog("[room-p2p] ready peer=%u elapsed=%llu",seat,now-p.started);}
  if(p.ready&&!ready){p.ready=false;p.failed=true;TodLog("[room-p2p] lost peer=%u fallback=server",seat);}
  if(!ready&&!p.failed&&now-p.started>=5000){p.failed=true;mPunchLink.DropRoomPeer(seat);TodLog("[room-p2p] timeout peer=%u fallback=server",seat);}
  if(!p.failed)discovering=true;
 }
 if(IsRelay()&&discovering&&mPunchAdvertised&&(!mPunchRelay||now-mPunchRenewAt>=90000)&&now-mPunchQueryAt>=1000){
  uint8_t p[5]{ObserverDirect::OP_CAPABLE};ObserverDirect::Put32(p+1,ObserverDirect::REVISION);
  SendRaw(ObserverDirect::CONTROL_TYPE,p,5);mPunchQueryAt=now;
 }
 for(uint8_t seat=1;seat<=6;++seat){auto&p=mFullWatch->peers[seat];
  if(!p.present||p.failed||mLocalSeat>seat||now-p.requested<1000)continue;
  if(p.ready&&now-p.renew<90000)continue;
  if(IsRelay()){
   if(!mPunchLink.HasEndpoint())continue;
   if(now-mFullWatch->pairWindow>=10000){mFullWatch->pairWindow=now;mFullWatch->pairRequests=0;}
   if(mFullWatch->pairRequests>=10)continue;++mFullWatch->pairRequests;
   uint8_t request[2]{ObserverDirect::OP_BEGIN,seat};SendRaw(ObserverDirect::CONTROL_TYPE,request,2);
  }else if(mTransport==Transport::DIRECT){
   if(!mPunchLink.Open())continue;
   uint8_t request[19]{5};ObserverDirect::Put16(request+1,mPunchLink.LocalPort());
   if(!ObserverDirect::DirectLink::Token(request+3))continue;
   // Direct LAN signaling is already authenticated by the game connection.
   SendRaw(ObserverDirect::CONTROL_TYPE,request,sizeof(request));
  }
  p.requested=now;
 }
 std::vector<ObserverDirect::DirectLink::Result> events;mPunchLink.Pump(now,&events);
 for(auto&e:events)if(IsRelay()){uint8_t p[21];ObserverDirect::PutResult(p,e.peerSeat,e.ok,e.rttMs);std::memcpy(p+5,e.token,16);SendRaw(ObserverDirect::CONTROL_TYPE,p,21);}
 ObserverDirect::Frame f;while(mPunchLink.Take(f)){
  if(!IsWatching()||f.context.src!=mWatchTargetSeat||f.context.dst!=mLocalSeat||f.context.watch!=mFullWatch->subscription||f.context.term!=mFlowTerm||f.context.epoch!=(ObserverEpoch()?ObserverEpoch():1))continue;
  ReceiveFullEnvelope(f.data.data(),int(f.data.size()),true);
 }
}
bool NetSession::HandleRoomPunch(const uint8_t*p,int n){
 if(!mObserverP2PEnabled||!IsConnected()||!p||n<1)return false;
 if(mTransport==Transport::DIRECT){
  auto peer=DirectPeerSeat();if(n!=19||p[0]!=5||!ObserverDirect::Get16(p+1)||!mPunchLink.Open())return false;
  auto& state=mFullWatch->peers[peer];if(state.failed)return false;
  ObserverDirect::Context dummy{};
  // StartProbe with no subscription permits duplex room data; LAN address
  // comes from the established TCP socket, never the untrusted message body.
  unsigned a,b,c,d;if(std::sscanf(mLink.GetPeerIPv4Text().c_str(),"%u.%u.%u.%u",&a,&b,&c,&d)!=4)return false;
  bool ok=mPunchLink.StartProbe(peer,a|(b<<8)|(c<<16)|(d<<24),ObserverDirect::Get16(p+1),p+3,ObserverNow());
  if(ok){state.renew=ObserverNow();if(mLocalSeat>peer){auto reply=std::vector<uint8_t>(p,p+n);ObserverDirect::Put16(reply.data()+1,mPunchLink.LocalPort());SendRaw(ObserverDirect::CONTROL_TYPE,reply.data(),int(reply.size()));}}
  return ok;
 }
 if(!mPunchAdvertised)return false;
 if(p[0]==ObserverDirect::OP_CAPABLE){ObserverDirect::CapableAck ack;
  if(!ObserverDirect::ParseCapableAck(p,n,&ack)||!mPunchLink.Open())return false;
  if(mPunchLink.StartHello(GetRelayHost().c_str(),ack.udpPort,ack.token,ObserverNow())){mPunchRelay=true;mPunchRenewAt=ObserverNow();return true;}return false;
 }
 ObserverDirect::Offer offer;
 if(n!=ObserverDirect::OFFER_SIZE||!ObserverDirect::ParseOffer(p,n,&offer)||offer.peerSeat==mLocalSeat||!IsSeatOccupied(offer.peerSeat)||IsSeatOffline(offer.peerSeat))return false;
 auto& state=mFullWatch->peers[offer.peerSeat];if(state.failed)return false;
 if(!mPunchLink.Open())return false;
 bool ok=mPunchLink.StartProbe(offer.peerSeat,offer.ip,offer.port,offer.token,ObserverNow(),nullptr,false,offer.ttl);
 if(ok){state.renew=ObserverNow();if(state.ready)state.started=ObserverNow();state.ready=false;}
 return ok;
}
