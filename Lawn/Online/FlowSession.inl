// Flow v1 is negotiated with the relay. It does not change direct connections.
bool NetSession::SendFlowProgress(uint8_t op,bool value,uint32_t generation) {
    if(!IsConnected() || !mFlowEnabled || !mReadyEpoch || IsWaitingForNextRound(mLocalSeat))return false;
    uint8_t bytes[14];NetProto::Writer w(bytes,sizeof(bytes));
    w.U32(mFlowTerm);w.U32(mReadyEpoch);w.U8(op);w.U8(value?1:0);w.U32(generation);
    return SendRaw(NetProto::MSG_CLI_FLOW_PROGRESS,bytes,sizeof(bytes));
}
bool NetSession::SendFlowStart(const NetProto::MsgStartLevel& msg) {
    uint8_t bytes[8+45];NetProto::Writer w(bytes,sizeof(bytes));w.U32(mFlowTerm);w.U32(msg.mReadyEpoch);
    int n=NetProto::EncodeStartLevel(bytes+8,45,msg);
    return n==45 && SendRaw(NetProto::MSG_CLI_FLOW_START,bytes,sizeof(bytes));
}
bool NetSession::SendFlowLevelDone(bool clear) {
    if(!IsConnected() || IsWaitingForNextRound(mLocalSeat) || mReadyEpoch!=mFlowEpoch)return false;
    if(mFlowDoneEpoch!=mReadyEpoch || mFlowDoneValue!=clear) {
        if(!SendFlowProgress(1,clear))return false;
        mFlowDoneEpoch=mReadyEpoch;mFlowDoneValue=clear;
        mSeats[mLocalSeat].mLevelDone=clear;
    }
    // Called only after the app rechecks its board, BOSSes and queued escapes.
    if(clear && mFlowPhase==3 && mFlowAcked!=mFlowGeneration) {
        if(!SendFlowProgress(2,true,mFlowGeneration))return false;
        mFlowAcked=mFlowGeneration;
    }
    return true;
}
void NetSession::HandleFlowState(const uint8_t* bytes,int size) {
    if(!IsRelay() || !IsConnected() || size!=24)return;
    NetProto::Reader r(bytes,size);
    uint16_t version=r.U16();uint8_t flags=r.U8(),phase=r.U8(),host=r.U8(),online=r.U8(),occupied=r.U8(),waiting=r.U8();
    uint32_t term=r.U32(),epoch=r.U32(),generation=r.U32();
    uint8_t done=r.U8(),acks=r.U8(),applied=r.U8(),reserved=r.U8();
    (void)acks;(void)applied;
    if(version!=1 || (flags&~3) || phase>5 || !term || term<mFlowTerm || host<1 || host>NetProto::MAX_PLAYERS ||
        (online&0x81) || (occupied&0x81) || (online&~occupied) || (waiting&~occupied) ||
        !(online&(1u<<mLocalSeat)) || !(online&(1u<<host)) || reserved)return;
    const bool changed=mFlowTerm && term!=mFlowTerm;
    const bool newHost=mHostSeat!=host;
    mFlowSupported=true;mFlowEnabled=(flags&1)!=0;mFlowLost=(flags&2)!=0;mFlowTerm=term;mFlowEpoch=epoch;
    mFlowPhase=phase;mFlowGeneration=generation;mRoundWaitMask=waiting;
    if(mFlowStartPending && epoch==mReadyEpoch)mFlowStartPending=false;
    for(uint8_t s=1;s<=NetProto::MAX_PLAYERS;++s) {
        mSeats[s].mOccupied=(occupied&(1u<<s))!=0;
        mSeats[s].mOffline=(occupied&(1u<<s)) && !(online&(1u<<s));
        if(mSeats[s].mOffline || (waiting&(1u<<s)))mSeats[s].mAwaitingAck=false;
        if(epoch==mReadyEpoch)mSeats[s].mLevelDone=(done&(1u<<s))!=0;
    }
    if(waiting&(1u<<mLocalSeat))mFlowDeferred=true;
    else if(phase==1 && epoch==mReadyEpoch && mConsumedStartEpoch==epoch)mFlowDeferred=false;
    mHostSeat=host;mRole=mLocalSeat==host?Role::HOST:Role::CLIENT;
    if(changed || newHost) {
        mFlowAcked=0;mFlowDoneEpoch=0;mFlowStartPending=false;
        mFlowHostStartRecovery=mRole==Role::HOST && phase==1 && !mPreparation.local;
        mHasRunGo=false;mHasPendingStart=false;mAnyAckRejected=false;
        mPendingZombieVoteOrder.clear();mPendingZombieVoteOpens.clear();mPendingZombieVotes.clear();mPendingZombieVoteResults.clear();
        mAdmissionInbox.clear();ClearSwapState();
        mGateWanted=-1;mGateRequest=mGateAck=0;mGateRetry=0;
        if(mReadyCancelState==ReadyCancelState::PENDING){mReadyCancelTerm=term;mReadyCancelRevision=0;}
        // Forget transaction consent, keep only the actual server-confirmed map.
        bool local=mPreparation.local,managed=mPreparation.gameManaged;
        mPreparation=PreparationWire::StateData{};mPreparation.local=local;
        mPreparation.gameManaged=managed;mPreparation.epoch=mReadyEpoch;
        if(phase>=2)mPreparation.phase=PreparationWire::GO;
        if(phase<2)for(uint8_t s=1;s<=NetProto::MAX_PLAYERS;++s)mCoopPeers[s]=s==mLocalSeat?mCoopToken:0;
        for(uint8_t s=1;s<=NetProto::MAX_PLAYERS;++s) {
            mSeats[s].mSeedsReady=false;mSeats[s].mAwaitingAck=false;
        }
        if(mFlowHostStartRecovery && mLastStartLevelValid) {
            mPendingStart=mLastStartLevel;mHasPendingStart=true;
            mLastStartLevel.mSrcSeat=mLocalSeat;
            for(uint8_t s=1;s<=NetProto::MAX_PLAYERS;++s)
                if(s!=mLocalSeat && (online&(1u<<s)) && !(waiting&(1u<<s)))ResendLastStartLevel(s);
        }
        PushEvent(EventType::HOST_CHANGED,host);
    }
    UpdateReadyCancelState();
    if(mFlowEnabled && !mFlowHelloSent && mRole==Role::CLIENT) {
        mFlowHelloSent=true;AdmissionWire::Message hello;hello.op=AdmissionWire::HELLO;
        SendAdmission(hello,mHostSeat);
    }
}

bool NetSession::CanCancelSeedsReady() const {
 return IsConnected() && IsPreparationActive() && !IsWaitingForNextRound(mLocalSeat) &&
  (mPreparation.phase==PreparationWire::OPEN || mPreparation.phase==PreparationWire::SEAL);
}
bool NetSession::RequestSeedsReadyCancel() {
 if(mReadyCancelState==ReadyCancelState::PENDING)return false;
 if(!CanCancelSeedsReady()){mReadyCancelState=ReadyCancelState::TOO_LATE;return false;}
 mReadyCancelState=ReadyCancelState::PENDING;mReadyCancelEpoch=mReadyEpoch;
 mReadyCancelTerm=mFlowTerm;mReadyCancelRevision=mPreparation.revision;
 if(!SendSeedsReady(false)){mReadyCancelState=ReadyCancelState::NONE;return false;}
 UpdateReadyCancelState();return true;
}
void NetSession::UpdateReadyCancelState() {
 if(mReadyCancelState!=ReadyCancelState::PENDING)return;
 if(mReadyEpoch!=mReadyCancelEpoch || mPreparation.phase==PreparationWire::GO || (mFlowEnabled && mFlowEpoch==mReadyCancelEpoch && mFlowPhase>=2)){mReadyCancelState=ReadyCancelState::TOO_LATE;return;}
 if(mFlowTerm!=mReadyCancelTerm)return;
 if(mPreparation.phase==PreparationWire::OPEN && mPreparation.revision>=mReadyCancelRevision &&
  !(mPreparation.ready&(1u<<mLocalSeat)))mReadyCancelState=ReadyCancelState::CONFIRMED;
}
