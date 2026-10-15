#include "CoopPlay.h"
#include "../LawnApp.h"
#include "Board.h"
#include "Plant.h"
#include "SeedPacket.h"
#include "Widget/LawnDialog.h"
#include "Widget/SeedChooserScreen.h"
#include "System/PlayerInfo.h"
#include "../Resources.h"
#include "ModText.h"
#include "graphics/Graphics.h"
#include "graphics/Font.h"
#include <algorithm>
#include <climits>
#ifdef __ANDROID__
#include "Widget/MobileCoopDraw.h"
#include "../PvzpLib/PvzpStringFile.h"
#else
#endif

static Board* CoopBoard(LawnApp* app) {
#ifdef __ANDROID__
    return app->mBoard;
#else
    return app->mBoard;
#endif
}
static NetSession* CoopSession(LawnApp* app) {
#ifdef __ANDROID__
    return app->mOnlineSession.get();
#else
    return app->mOnlineSession;
#endif
}
static void CoopText(Graphics* g,const std::string& text,int x,int y,int w,int h,int size=18) {
#ifdef __ANDROID__
    PvzpDrawStringWrapped(g,text,Rect(x,y,w,h),ModText::FontFor(FONT_DWARVENTODCRAFT18,size),Color(245,230,165),DS_ALIGN_LEFT);
#else
    auto* font=ModText::GetFont(size*3/4,false);auto wide=ModText::WideFromUtf8(text.c_str());
    int row=y;std::wstring line;
    for(wchar_t ch:wide) {
        if(ch=='\n' || (!line.empty() && ModText::TextWidth(font,line+ch)>w)) {
            if(row+size>y+h)break;
            ModText::DrawTextWide(g,font,x,row,line,Color(245,230,165),g->mClipRect);line.clear();row+=size+3;
            if(ch=='\n')continue;
        }
        line+=ch;
    }
    if(row+size<=y+h)ModText::DrawTextWide(g,font,x,row,line,Color(245,230,165),g->mClipRect);
#endif
}
static void CoopButton(Graphics* g,Rect rect,const std::string& text,bool enabled=true) {
#ifdef __ANDROID__
    DrawMobileStone(g,rect,text,false,enabled,18);
#else
    Graphics stone(*g);stone.SetColorizeImages(false);
    Image* left=IMAGE_BUTTON_LEFT;Image* middle=IMAGE_BUTTON_MIDDLE;Image* right=IMAGE_BUTTON_RIGHT;
    stone.DrawImage(left,Rect(rect.mX,rect.mY,left->mWidth,rect.mHeight),Rect(0,0,left->mWidth,left->mHeight));
    stone.DrawImage(middle,Rect(rect.mX+left->mWidth,rect.mY,rect.mWidth-left->mWidth-right->mWidth,rect.mHeight),Rect(0,0,middle->mWidth,middle->mHeight));
    stone.DrawImage(right,Rect(rect.mX+rect.mWidth-right->mWidth,rect.mY,right->mWidth,rect.mHeight),Rect(0,0,right->mWidth,right->mHeight));
    CoopText(g,text,rect.mX+10,rect.mY+10,rect.mWidth-20,rect.mHeight-10,18);
#endif
}

class CoopDialog : public LawnDialog {
    CoopPlay* coop;int page=0,plantPage=0,down=-1;uint64_t peer=0;
    Rect Box(int i) const{return Rect(45+(i%2)*360,180+(i/2)*62,340,52);}
    int Hit(int x,int y) {
        if(Rect(650,515,100,46).Contains(x,y))return 90;
        if(Rect(45,515,160,46).Contains(x,y))return 91;
        if(coop->hasIncoming) {
            if(Rect(210,450,170,48).Contains(x,y))return 80;
            if(Rect(420,450,170,48).Contains(x,y))return 81;
            return -1;
        }
        if(page==1) {
            for(int i=0;i<8;++i)if(Box(i).Contains(x,y))return i;
            if(Rect(240,450,140,48).Contains(x,y))return 82;
            if(Rect(420,450,140,48).Contains(x,y))return 83;
        } else {
            for(int i=0;i<NetProto::MAX_PLAYERS;++i)if(Box(i).Contains(x,y))return i;
            if(Rect(240,450,320,48).Contains(x,y))return 84;
        }
        return -1;
    }
public:
    explicit CoopDialog(CoopPlay* c):LawnDialog(c->app,DIALOG_COOP_PLAY,true,"","","",Dialog::BUTTONS_NONE),coop(c){Resize(0,0,800,600);}
    void Draw(Graphics* g) override {
        LawnDialog::Draw(g);
        auto* session=CoopSession(coop->app);auto* run=coop->app->GetRunState();
        CoopText(g,ModText::Tr("队友协作 · 借用与求援","Teamwork · loans and sun"),45,40,700,42,26);
        std::string info=coop->notice;
        if(run && run->mLoanLevel==run->mLevelIndex && run->mLoanPlant>=0) {
            // docs/09 §三.8：借入展示来源玩家、有效词条（自己层数 vs 借用后生效层数）与期限。
            info+=std::string("\n")+ModText::Tr("本关借入：","Borrowed this level: ")+GetRunPlantName((SeedType)run->mLoanPlant);
            if(!coop->mLoanLenderName.empty())info+=ModText::Tr("（来自 "," (from ")+coop->mLoanLenderName+")";
            for(const auto& b:run->mLoanBuffs) {
                int own=run->GetBuffCount(b.mId);int eff=own>b.mCount?own:b.mCount;
                info+=std::string("\n")+GetRunChoiceDisplayName(b.mId)+" x"+std::to_string(eff)
                    +ModText::Tr("（自己 x"," (own x")+std::to_string(own)+")";
            }
            info+=std::string("\n")+ModText::Tr("本关有效，下一关恢复自己的词条","Valid this level; your own traits return next level");
        }
        CoopText(g,info,45,91,700,75,18);
        if(coop->hasIncoming) {
            const auto& e=coop->incoming;std::string name=session->GetSeatName(e.src);
            CoopText(g,name+ModText::Tr("申请借用："," asks to borrow: ")+GetRunPlantName((SeedType)e.plant),55,185,690,55,24);
            std::string traits=ModText::Tr("同意后共享你这株的专属词条，本关你仍可使用。\n","Share this plant's permanent traits for this level; you keep using it.\n");
            if(run)for(const auto& b:run->mBuffs)if(GetRunChoicePlant(b.mId)==(SeedType)e.plant)
                traits+=std::string(GetRunChoiceDisplayName(b.mId))+" x"+std::to_string(b.mCount)+"\n";
            CoopText(g,traits,55,250,690,185);
            CoopButton(g,Rect(210,450,170,48),ModText::Tr("同意借出","Accept"));
            CoopButton(g,Rect(420,450,170,48),ModText::Tr("拒绝","Decline"));
        } else if(page==1) {
            for(int i=0;i<8;++i) {
                int plant=plantPage*8+i;
                CoopButton(g,Box(i),GetRunPlantName((SeedType)plant),coop->Preparing()&&!coop->loanWaiting);
            }
            CoopButton(g,Rect(240,450,140,48),ModText::Tr("上一页","Previous"));
            CoopButton(g,Rect(420,450,140,48),ModText::Tr("下一页","Next"));
            CoopText(g,ModText::Tr("申请后由队友确认；队友须拥有该植物。","Your teammate must own the plant and approve."),55,420,690,26);
        } else {
            for(int i=0;i<NetProto::MAX_PLAYERS;++i) {
                uint8_t seat=i+1;bool valid=session && session->IsSeatOccupied(seat)&&seat!=session->GetLocalSeat();
                uint64_t token=session?session->mCoopPeers[seat]:0;
                std::string label=valid?session->GetSeatName(seat):ModText::Tr("空席 / 自己","Empty / you");
                if(valid)label+=coop->requests.count(token)?ModText::Tr(" · 赠送90阳光"," · send 90 sun"):ModText::Tr(" · 申请借用"," · borrow");
                CoopButton(g,Box(i),label,valid&&token);
            }
            CoopButton(g,Rect(240,450,320,48),ModText::Tr("请求一份阳光","Request one sun gift"),coop->requestCooldown==0 && coop->app->mGameScene==SCENE_PLAYING);
        }
        CoopButton(g,Rect(45,515,160,46),ModText::Tr("返回列表","Team list"));
        CoopButton(g,Rect(650,515,100,46),ModText::Tr("关闭","Close"));
    }
    void MouseDown(int x,int y,int clicks) override {down=clicks>0?Hit(x,y):-1;}
    void MouseUp(int x,int y,int clicks) override {
        int action=Hit(x,y);if(clicks<=0 || down<0 || action!=down)return;down=-1;
        if(action==90){coop->Close();return;}
        if(action==91){page=0;return;}
        if(action==80 || action==81){coop->Answer(action==80);return;}
        if(action==82){plantPage=(plantPage+5)%6;return;}
        if(action==83){plantPage=(plantPage+1)%6;return;}
        if(action==84){coop->RequestSun();return;}
        auto* session=CoopSession(coop->app);
        if(page==1){if(coop->Ask(peer,(SeedType)(plantPage*8+action)))page=0;return;}
        if(action<0 || action>=NetProto::MAX_PLAYERS || !session)return;
        uint8_t seat=action+1;if(seat==session->GetLocalSeat()||!session->IsSeatOccupied(seat))return;
        peer=session->mCoopPeers[seat];if(!peer)return;
        if(coop->requests.count(peer)){coop->Aid(peer);return;}
        if(coop->Preparing()){page=1;plantPage=0;}else coop->notice=ModText::Tr("借用仅在关间奖励或选卡期间开放","Borrow during rewards or seed selection");
    }
    void KeyDown(KeyCode key) override {if(key==KEYCODE_ESCAPE)coop->Close();}
};

bool CoopPlay::Preparing() const {
    auto* run=app->GetRunState();
    return app->IsOnlineGame() && run && app->mGameScene==SCENE_LEVEL_INTRO &&
        (run->HasPendingPick() || app->mSeedChooserScreen!=nullptr);
}
bool CoopPlay::Blocking() const{return app->GetDialog(DIALOG_COOP_PLAY)!=nullptr || hasIncoming || loanWaiting;}
void CoopPlay::Show(){auto* session=CoopSession(app);if(session&&session->IsConnected()&&app->GetRunState()&&!app->GetDialog(DIALOG_COOP_PLAY))app->AddDialog(DIALOG_COOP_PLAY,new CoopDialog(this));}
CoopWire::Event CoopPlay::Message(uint8_t op) {
    CoopWire::Event e;e.op=op;e.serial=++serial;
    if(auto* run=app->GetRunState()){e.run=(uint32_t)run->mRunSeed;e.level=run->mLevelIndex;}return e;
}
bool CoopPlay::Send(CoopWire::Event event,uint64_t target) {
    auto* session=CoopSession(app);if(!session)return false;
    if(target){uint8_t seat=session->FindCoopSeat(target);if(!seat)return false;event.target=target;return session->SendCoop(event,seat);}
    return session->SendCoop(event);
}
bool CoopPlay::RequestSun() {
    auto* b=CoopBoard(app);if(!b||app->mGameScene!=SCENE_PLAYING || requestCooldown>0){notice=ModText::Tr("战斗中可求援；请等待请求冷却结束","Request sun during battle after its cooldown");return false;}
    auto e=Message(CoopWire::SUN_REQUEST);if(!Send(e)){notice=ModText::Tr("求援未发送","Request could not be sent");return false;}
    requestId=e.serial;requestGrace=0;requestTicks=1000;requestCooldown=1500;notice=ModText::Tr("已求援：十秒内可响应一份阳光","Sun requested for 10 seconds");return true;
}
bool CoopPlay::Aid(uint64_t token) {
    auto* b=CoopBoard(app);auto it=requests.find(token);
    if(!b || app->mGameScene!=SCENE_PLAYING || it==requests.end() || giftWaiting || sunCooldown>0 ||
        b->mSunGiftCooldownFrames>0 || b->mSunMoney<100){notice=ModText::Tr("余额不足、冷却中或请求已结束","Not enough sun, cooling down, or request ended");return false;}
    gift=Message(CoopWire::SUN_GIFT);gift.request=it->second.serial;gift.target=token;
    b->mSunMoney-=100;
    if(!Send(gift,token)){b->mSunMoney+=100;notice=ModText::Tr("发送失败，阳光已退回","Send failed; sun returned");return false;}
    giftWaiting=true;giftTicks=0;sunCooldown=300;b->mSunGiftCooldownFrames=300;
    notice=ModText::Tr("赠送待确认","Gift awaiting confirmation");return true;
}
void CoopPlay::SunReceived() {
    if(!requestId)return;
    auto e=Message(CoopWire::SUN_DONE);e.request=requestId;Send(e);requestTicks=0;requestGrace=100;
}
bool CoopPlay::Ask(uint64_t token,SeedType plant) {
    auto* run=app->GetRunState();
    if(!Preparing()||!run||loanWaiting||run->mLoanLevel==run->mLevelIndex){notice=ModText::Tr("请在准备阶段申请，每关限借入一株","Borrow during preparation; one plant per level");return false;}
    auto e=Message(CoopWire::ASK);e.plant=(uint8_t)plant;e.target=token;
    if(!Send(e,token))return false;
    pendingLoan=e;loanWaiting=true;loanTicks=1200;notice=ModText::Tr("借用申请已发出，等待队友确认","Loan requested; waiting for approval");return true;
}
void CoopPlay::Answer(bool accept) {
    if(!hasIncoming)return;
    auto request=incoming;hasIncoming=false;
    auto* run=app->GetRunState();bool valid=accept&&Preparing()&&run&&run->HasPlant((SeedType)request.plant)&&run->mLentLevel!=run->mLevelIndex;
    auto e=Message(valid?CoopWire::OFFER:CoopWire::DECLINE);e.request=request.serial;e.plant=request.plant;
    if(valid) {
        for(const auto& buff:run->mBuffs)if(GetRunChoicePlant(buff.mId)==(SeedType)e.plant) {
            if(e.count>=CoopWire::MAX_TRAITS){e.op=CoopWire::DECLINE;valid=false;break;}
            e.ids[e.count]=buff.mId;e.stacks[e.count++]=buff.mCount;
        }
    }
    if(Send(e,request.sender)&&valid){lentPeer=request.sender;lentRequest=request.serial;lentCommitted=false;run->mLentLevel=run->mLevelIndex;run->Save(app->mPlayerInfo->mId);}
    notice=valid?ModText::Tr("已同意，本关仍可正常使用自己的植物","Approved; your own plant remains usable"):ModText::Tr("已拒绝或当前不能借出","Declined or unavailable");
}
void CoopPlay::Capture(std::vector<int>& health,std::vector<float>& attack,std::vector<int>& refresh) {
    auto* b=CoopBoard(app);if(!b)return;
    Plant* p=nullptr;while(b->IteratePlants(p)){health.push_back(p->CoopMaxHealth());attack.push_back(p->CoopAttackPeriod());}
    for(int i=0;i<b->mSeedBank->mNumPackets;++i){auto& card=b->mSeedBank->mSeedPackets[i];
        refresh.push_back(std::max(1,(int)(Plant::GetRefreshTime(card.mPacketType,card.mImitaterType)*app->RunBuffMul(RUN_BUFF_FASTSEED)*app->RunPlantUpgradeMulKind(card.mPacketType,RUN_UPGRADE_KIND_COOLDOWN)*app->RunCoffeeBeanRefreshMul(card.mPacketType,card.mImitaterType))));}
    if(app->mSeedChooserScreen)for(int seed=0;seed<48;++seed)
        refresh.push_back(std::max(1,(int)(Plant::GetRefreshTime((SeedType)seed,SEED_NONE)*app->RunBuffMul(RUN_BUFF_FASTSEED)*app->RunPlantUpgradeMulKind((SeedType)seed,RUN_UPGRADE_KIND_COOLDOWN)*app->RunCoffeeBeanRefreshMul((SeedType)seed,SEED_NONE))));
}
void CoopPlay::Rescale(const std::vector<int>& health,const std::vector<float>& attack,const std::vector<int>& refresh) {
    auto* b=CoopBoard(app);if(!b)return;Plant* p=nullptr;size_t i=0;
    while(b->IteratePlants(p)&&i<health.size()) {
        int expected=p->CoopMaxHealth();if(expected!=health[i]) {
            int maximum=(int)std::min((long long)INT_MAX,(long long)p->mPlantMaxHealth*expected/std::max(1,health[i]));maximum=std::max(1,maximum);
            p->mPlantHealth=(int)((long long)std::max(0,p->mPlantHealth)*maximum/std::max(1,p->mPlantMaxHealth));p->mPlantMaxHealth=maximum;
        }
        float period=p->CoopAttackPeriod();if(p->mLaunchCounter>0 && attack[i]>0 && period!=attack[i])p->mLaunchCounter=std::max(0,(int)(p->mLaunchCounter*(double)period/attack[i]));++i;
    }
    for(int j=0;j<b->mSeedBank->mNumPackets&&j<(int)refresh.size();++j) {
        auto& c=b->mSeedBank->mSeedPackets[j];int period=std::max(1,(int)(Plant::GetRefreshTime(c.mPacketType,c.mImitaterType)*app->RunBuffMul(RUN_BUFF_FASTSEED)*app->RunPlantUpgradeMulKind(c.mPacketType,RUN_UPGRADE_KIND_COOLDOWN)*app->RunCoffeeBeanRefreshMul(c.mPacketType,c.mImitaterType)));
        if(c.mRefreshing && period!=refresh[j]){c.mRefreshCounter=(int)((long long)c.mRefreshCounter*period/refresh[j]);c.mRefreshTime=period;}
    }
    if(app->mSeedChooserScreen)for(int seed=0;seed<48;++seed) {
        auto& chosen=app->mSeedChooserScreen->mChosenSeeds[seed];size_t index=b->mSeedBank->mNumPackets+seed;
        int period=std::max(1,(int)(Plant::GetRefreshTime((SeedType)seed,SEED_NONE)*app->RunBuffMul(RUN_BUFF_FASTSEED)*app->RunPlantUpgradeMulKind((SeedType)seed,RUN_UPGRADE_KIND_COOLDOWN)*app->RunCoffeeBeanRefreshMul((SeedType)seed,SEED_NONE)));
        if(chosen.mRefreshing && index<refresh.size() && refresh[index]>0 && period!=refresh[index])
            chosen.mRefreshCounter=(int)((long long)chosen.mRefreshCounter*period/refresh[index]);
    }
}
void CoopPlay::ApplyLoan(const CoopWire::Event& e) {
    auto* run=app->GetRunState();if(!run)return;
    auto* session=CoopSession(app);
    mLoanLenderName=session?session->GetSeatName(e.src):std::string();
    if(mLoanLenderName.empty())mLoanLenderName="P"+std::to_string((unsigned)e.src);
    std::vector<RunState::BuffStack> buffs;for(int i=0;i<e.count;++i){RunState::BuffStack b;b.mId=e.ids[i];b.mCount=e.stacks[i];buffs.push_back(b);}
    if(!run->ValidateLoan((SeedType)e.plant,buffs)){notice=ModText::Tr("借用取消：词条互斥或数据不兼容","Loan declined: conflicting or incompatible traits");auto reply=Message(CoopWire::DECLINE);reply.request=e.request;Send(reply,e.sender);return;}
    std::vector<int> hp,refresh;std::vector<float> attack;Capture(hp,attack,refresh);
    run->mLoanPlant=e.plant;run->mLoanLevel=run->mLevelIndex;run->mLoanBuffs=buffs;Rescale(hp,attack,refresh);
    if(auto* b=CoopBoard(app)) {
        auto pool=run->GetUsablePool();int before=b->mSeedBank->mNumPackets;
        if(pool.size()<=RunState::RUN_SEED_SLOTS && before<(int)pool.size()) {
            for(int i=before;i<(int)pool.size();++i)b->mSeedBank->mSeedPackets[i].SetPacketType(pool[i]);
            b->mSeedBank->mNumPackets=(int)pool.size();b->mSeedBank->UpdateWidth();
        }
    }
    run->Save(app->mPlayerInfo->mId);notice=ModText::Tr("借用成功：同种植物各词条取双方最高层","Borrowed: same-plant traits use the higher stacks");
    auto reply=Message(CoopWire::ACCEPT);reply.request=e.request;reply.plant=e.plant;Send(reply,e.sender);
}
void CoopPlay::ExpireLoan() {
    auto* run=app->GetRunState();if(!run || run->mLoanLevel!=run->mLevelIndex)return;
    std::vector<int> hp,refresh;std::vector<float> attack;Capture(hp,attack,refresh);
    run->mLoanPlant=run->mLoanLevel=-1;run->mLoanBuffs.clear();Rescale(hp,attack,refresh);
    mLoanLenderName.clear();
}
void CoopPlay::Handle(const CoopWire::Event& e) {
    auto* session=CoopSession(app);auto* run=app->GetRunState();auto* board=CoopBoard(app);
    if(e.op==CoopWire::HELLO)return;
    if(e.op==CoopWire::SUN_RESULT) {
        if(giftWaiting && e.sender==gift.target && e.request==gift.serial) {
            bool refunded=!e.result && board && run && (uint32_t)run->mRunSeed==gift.run && (uint32_t)run->mLevelIndex==gift.level;
            if(refunded){board->mSunMoney=std::min(9990,board->mSunMoney+100);sunCooldown=0;board->mSunGiftCooldownFrames=0;}
            giftWaiting=false;notice=e.result?ModText::Tr("队友已收到90阳光","Teammate received 90 sun"):
                refunded?ModText::Tr("未到账，阳光已退回","Not delivered; sun returned"):ModText::Tr("未到账，原关卡已结束","Not delivered; original level ended");
        }return;
    }
    if(e.op==CoopWire::SUN_GIFT) {
        auto key=std::make_pair(e.sender,e.serial);auto it=receipts.find(key);
        if(it!=receipts.end()){Send(it->second,e.sender);return;}
        auto reply=Message(CoopWire::SUN_RESULT);reply.request=e.serial;
        reply.result=run&&board&&app->mGameScene==SCENE_PLAYING&&e.run==(uint32_t)run->mRunSeed&&e.level==(uint32_t)run->mLevelIndex&&e.request==requestId && (requestTicks>0 || requestGrace>0);
        if(receipts.size()>=128)receipts.erase(receipts.begin());
        if(giftHigh[e.sender]>=e.serial)reply.result=1;
        else if(reply.result){giftHigh[e.sender]=e.serial;board->GiveSunGift(90,e.src);}
        receipts[key]=reply;
        Send(reply,e.sender);return;
    }
    if(!run||e.run!=(uint32_t)run->mRunSeed||e.level!=(uint32_t)run->mLevelIndex)return;
    if(e.op==CoopWire::SUN_REQUEST) {
        if(app->mGameScene==SCENE_PLAYING){requests[e.sender]=e;requestLife[e.sender]=1000;
            if(board)board->PushCustomBanner((session->GetSeatName(e.src)+ModText::Tr("请求阳光 · 按B响应"," requests sun · B to respond")).c_str());}
    } else if(e.op==CoopWire::SUN_DONE) {
        auto it=requests.find(e.sender);if(it!=requests.end()&&it->second.serial==e.request){requests.erase(it);requestLife.erase(e.sender);}
    } else if(e.op==CoopWire::ASK) {
        if(!Preparing()||hasIncoming||!run->HasPlant((SeedType)e.plant)||run->mLentLevel==run->mLevelIndex) {
            auto reply=Message(CoopWire::DECLINE);reply.request=e.serial;Send(reply,e.sender);return;
        }
        incoming=e;hasIncoming=true;incomingTicks=1200;Show();
    } else if(e.op==CoopWire::OFFER) {
        if(loanWaiting&&Preparing()&&e.sender==pendingLoan.target&&e.request==pendingLoan.serial&&e.plant==pendingLoan.plant) {
            loanWaiting=false;ApplyLoan(e);
        } else {auto reply=Message(CoopWire::DECLINE);reply.request=e.request;Send(reply,e.sender);}
    } else if(e.op==CoopWire::ACCEPT) {
        if(e.sender==lentPeer && e.request==lentRequest){lentCommitted=true;notice=ModText::Tr("队友已确认借入","Teammate confirmed the loan");}
    } else if(e.op==CoopWire::DECLINE) {
        if(!lentCommitted && e.sender==lentPeer && e.request==lentRequest){run->mLentLevel=-1;lentPeer=0;run->Save(app->mPlayerInfo->mId);}
        if(loanWaiting&&e.sender==pendingLoan.target&&e.request==pendingLoan.serial){loanWaiting=false;notice=ModText::Tr("队友拒绝或当前无法借出","Teammate declined or cannot lend now");}
    } else if(e.op==CoopWire::CANCEL) {
        if(hasIncoming&&e.sender==incoming.sender&&e.request==incoming.serial)hasIncoming=false;
    }
}
void CoopPlay::Update() {
    auto* session=CoopSession(app);auto* run=app->GetRunState();
    if(!run || !session||!session->IsConnected()) {
        if(!run)giftWaiting=false;
        hasIncoming=loanWaiting=false;requests.clear();requestLife.clear();requestTicks=0;requestGrace=0;
        if(app->GetDialog(DIALOG_COOP_PLAY))app->KillDialog(DIALOG_COOP_PLAY);return;
    }
    if(observedToken!=session->mCoopToken){observedToken=session->mCoopToken;giftWaiting=false;receipts.clear();giftHigh.clear();requestId=0;requestGrace=0;}
    if(app->mGameScene!=SCENE_PLAYING){requests.clear();requestLife.clear();requestId=0;requestTicks=requestGrace=0;giftWaiting=false;}
    if(--helloTicks<=0){helloTicks=100;Send(Message(CoopWire::HELLO));}
    if(run && (observedRun!=run->mRunSeed||observedLevel!=run->mLevelIndex)) {
        observedRun=run->mRunSeed;observedLevel=run->mLevelIndex;hasIncoming=loanWaiting=false;requests.clear();requestLife.clear();requestTicks=0;requestId=0;requestGrace=0;giftWaiting=false;mLoanLenderName.clear();
        if(app->GetDialog(DIALOG_COOP_PLAY))app->KillDialog(DIALOG_COOP_PLAY);
    }
    if(requestGrace>0)--requestGrace;
    if(requestCooldown>0)--requestCooldown;if(sunCooldown>0)--sunCooldown;
    if(requestTicks>0 && --requestTicks==0){auto e=Message(CoopWire::SUN_DONE);e.request=requestId;Send(e);}
    for(auto it=requestLife.begin();it!=requestLife.end();) {
        if(--it->second<=0||!session->FindCoopSeat(it->first)){requests.erase(it->first);it=requestLife.erase(it);}else ++it;
    }
    if(hasIncoming && (--incomingTicks<=0||!Preparing()))Answer(false);
    if(loanWaiting && (--loanTicks<=0||!Preparing())){auto e=Message(CoopWire::CANCEL);e.request=pendingLoan.serial;Send(e,pendingLoan.target);loanWaiting=false;notice=ModText::Tr("借用申请已结束","Loan request ended");}
    if(giftWaiting && ++giftTicks%100==0)Send(gift,gift.target); // retry same identity; no refund on uncertainty
    CoopWire::Event event;while(session->TakeCoop(event))Handle(event);
}

void CoopPlay::DrawEntry(Graphics* g,int x,int y,int w,int h,bool theFlat)
{
    auto* session=CoopSession(app);
    if(session && session->IsConnected() && app->GetRunState()) {
        // 2026-10-15 用户令：局内的协作入口不用石头框，跟横幅/面板同款黑底细边精简样式
        //（奖励/选卡页仍用石钮，与所在页面的按钮风格一致）；两端共享本分支。
        if(theFlat) {
            g->SetColor(Color(0,0,0,150));
            g->FillRect(x,y,w,h);
            g->SetColor(Color(255,255,255,255));
            g->DrawRect(x,y,w-1,h-1);
            CoopText(g,ModText::Tr("协作 / 借用 B","Teamwork B"),x+10,y+10,w-20,h-10,18);
        } else {
            CoopButton(g,Rect(x,y,w,h),ModText::Tr("协作 / 借用 B","Teamwork B"));
        }
    }
}
bool LawnApp::SendCoopQuickChat(uint8_t id)
{
    if(!mCoopPlay)mCoopPlay=std::make_unique<CoopPlay>(this);
    if(id==9 && !mCoopPlay->RequestSun()) {
        if(auto* b=CoopBoard(this))b->PushCustomBanner(mCoopPlay->notice.c_str());return false;
    }
    auto* session=CoopSession(this);return session && session->SendQuickChat(id);
}

void CoopPlay::Close()
{
    if(hasIncoming)Answer(false);
    if(loanWaiting){auto e=Message(CoopWire::CANCEL);e.request=pendingLoan.serial;Send(e,pendingLoan.target);loanWaiting=false;}
    app->KillDialog(DIALOG_COOP_PLAY);
}
