namespace Sexy {class Graphics;}
#pragma once
#include "Online/CoopProtocol.h"
#include "Run/RunState.h"
#include <map>
#include <string>
#include <vector>
class LawnApp;class Board;
class CoopPlay {
public:
    explicit CoopPlay(LawnApp* owner):app(owner){}
    LawnApp* app;
    CoopWire::Event incoming{},pendingLoan{},gift{};
    bool hasIncoming=false,loanWaiting=false,giftWaiting=false;
    int incomingTicks=0,loanTicks=0,giftTicks=0,requestTicks=0,requestCooldown=0,helloTicks=0,sunCooldown=0;
    uint32_t serial=0,requestId=0,lentRequest=0;
    uint64_t lentPeer=0,observedToken=0;
    bool lentCommitted=false;
    int requestGrace=0;
    std::map<uint64_t,uint32_t> giftHigh;
    std::map<uint64_t,CoopWire::Event> requests;
    std::map<uint64_t,int> requestLife;
    std::map<std::pair<uint64_t,uint32_t>,CoopWire::Event> receipts;
    std::map<std::pair<uint64_t,uint32_t>,CoopWire::Event> pendingCancels;
    std::string notice;
    std::string mLoanLenderName;	// 借入来源显示名（内存态；借出者席位换人不追，docs/09 §三.8）
    int observedRun=0,observedLevel=-1;
    void Update();void Show();void Close();void DrawEntry(Sexy::Graphics* g,int x,int y,int w=140,int h=40,bool theFlat=false);bool Preparing() const;bool Blocking() const;
    bool RequestSun();bool Aid(uint64_t token);void SunReceived();
    bool GiveSun(uint8_t seat);
    bool SendGift(uint64_t token,uint32_t request,bool proactive);
    bool Ask(uint64_t token,SeedType plant);void Answer(bool accept);
    void CancelLoanRequest();
    bool ReleasePendingLend(uint64_t peer,uint32_t request);
    void ExpireLoan();void ApplyLoan(const CoopWire::Event& event);
    CoopWire::Event Message(uint8_t op);
    bool Send(CoopWire::Event event,uint64_t target=0);
    void Handle(const CoopWire::Event& event);
    void Rescale(const std::vector<int>& health,const std::vector<float>& attack,const std::vector<int>& refresh);
    void Capture(std::vector<int>& health,std::vector<float>& attack,std::vector<int>& refresh);
};
