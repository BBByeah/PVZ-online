#pragma once
#include "LawnDialog.h"
#include "../../LawnApp.h"
#include "OnlineUiDraw.h"
#include "../Run/RunHistory.h"
#include "../Run/EndlessRules.h"
#include "../System/PlayerInfo.h"
#include <ctime>

class RunHistoryDialog : public LawnDialog {
    LawnApp* app;
    std::vector<RunHistory::Record> records;
    RunHistory::ReadState readState;
    int filter=0,page=0,selected=-1,down=-1;
    std::vector<int> Visible()const {
        std::vector<int> result;
        for(int i=0;i<int(records.size());++i)if(!filter || (records[i].settings[RunHistory::MODE]==3)==(filter==2))result.push_back(i);
        return result;
    }
    static Rect Row(int i){return Rect(48,146+i*60,704,54);}
    static Rect Back(){return Rect(48,528,144,46);}
    static Rect Prev(){return Rect(216,528,112,46);}
    static Rect Next(){return Rect(344,528,112,46);}
    static Rect Filter(int i){return Rect(48+i*240,88,224,40);}
    static std::string Time(uint64_t value) {
        std::time_t stamp=std::time_t(value);std::tm local{};
#ifdef _WIN32
        localtime_s(&local,&stamp);
#else
        localtime_r(&stamp,&local);
#endif
        char out[32];std::strftime(out,sizeof(out),"%Y-%m-%d %H:%M",&local);return out;
    }
    static std::string Mode(const RunHistory::Record& r) {
        static const char* zh[]={"完整闯关","普通闯关","快速闯关","无尽"};
        static const char* en[]={"Full run","Normal run","Quick run","Endless"};
        static const char* scenesZh[]={"白天","夜晚","泳池","浓雾","屋顶"};
        static const char* scenesEn[]={"Day","Night","Pool","Fog","Roof"};
        auto s=std::string(ModText::Tr(zh[r.settings[0]],en[r.settings[0]]));
        if(r.settings[0]==3)s+=" · "+std::string(ModText::Tr(scenesZh[r.settings[1]],scenesEn[r.settings[1]]));return s;
    }
    static const char* Status(const RunHistory::Record& r) {
        switch(r.status){case RunHistory::FAILED:return ModText::Tr("上次失败","Last attempt lost");
            case RunHistory::COMPLETE:return ModText::Tr("已通关","Completed");case RunHistory::LEFT:return ModText::Tr("已离开","Left run");
            default:return ModText::Tr("已保存进度","Progress saved");}
    }
    int Hit(int x,int y) {
        if(Back().Contains(x,y))return 100;
        auto list=Visible();
        if(selected>=0){if(Prev().Contains(x,y))return 104;return -1;}
        if(Prev().Contains(x,y)&&page>0)return 101;
        if(Next().Contains(x,y)&&(page+1)*5<int(list.size()))return 102;
        for(int i=0;i<3;++i)if(Filter(i).Contains(x,y))return 110+i;
        for(int i=0;i<5 && page*5+i<int(list.size());++i)if(Row(i).Contains(x,y))return list[page*5+i];
        return -1;
    }
public:
    explicit RunHistoryDialog(LawnApp* a):LawnDialog(a,DIALOG_RUN_HISTORY,true,"","","",Dialog::BUTTONS_NONE),app(a) {
        Resize(0,0,800,600);readState=RunHistory::Read(app->mPlayerInfo->mId,records);
    }
    void Draw(Graphics* g) override {
        SeatUi::Panel(g,Rect(0,0,800,600));
        SeatUi::Text(g,ModText::Tr("本地最近记录","Recent local runs"),48,72,704,29,Color(245,230,165),true);
        if(selected>=0) {
            const auto& r=records[selected];using namespace RunHistory;
            SeatUi::Text(g,Mode(r)+" · "+Status(r),48,115,704,23,Color(245,230,165),true);
            std::vector<std::string> lines={
                ModText::Tr("开始记录：","First recorded: ")+Time(r.started),
                ModText::Tr("最近更新：","Last updated: ")+Time(r.updated),
                ModText::Tr("最高到达第 ","Furthest stage ")+std::to_string(r.stage)+ModText::Tr(" 关；加入时第 ","; joined at ")+std::to_string(r.joined),
                ModText::Tr("最多人数 ","Max players ")+std::to_string(r.players)+" / 6   ·   P"+std::to_string(r.seat)+ModText::Tr("   ·   失败 ","   ·   Losses ")+std::to_string(r.failures),
                ModText::Tr("本人总伤害 ","Your total damage ")+std::to_string(r.totalDamage)+ModText::Tr("；首领伤害 ","; boss damage ")+std::to_string(r.bossDamage),
                r.partial?ModText::Tr("伤害记录不完整，仅包含已记录部分。","Damage is partial; only recorded participation is included."):ModText::Tr("伤害只计本人实际参与。","Damage covers your actual participation."),
                ModText::Tr("种子 ","Seed ")+std::to_string(r.seed)+ModText::Tr("   ·   植物 ","   ·   Plants ")+std::to_string(r.pool)+ModText::Tr("   ·   增益种类 ","   ·   Buff types ")+std::to_string(r.buffs),
                ModText::Tr("难度：","Difficulty: ")+std::string(r.settings[DIFF]==0?ModText::Tr("轻松","Easy"):r.settings[DIFF]==1?ModText::Tr("标准","Standard"):ModText::Tr("高压","Hard"))+
                    ModText::Tr("   规模 ×","   Scale x")+(r.settings[SCALE]==0?"0.5":r.settings[SCALE]==1?"1":r.settings[SCALE]==2?"2":"4")+
                    ModText::Tr("   节奏：","   Tempo: ")+(r.settings[TEMPO]==0?ModText::Tr("快","Fast"):r.settings[TEMPO]==1?ModText::Tr("标准","Standard"):ModText::Tr("慢","Slow")),
                ModText::Tr("增益倍选 ×","Buff picks x")+std::to_string(r.settings[BUFFMULT]+1)+ModText::Tr("   ·   植物僵尸：","   ·   Zombotany: ")+ModText::Tr(r.settings[ZOMBOTANY]?"开":"关",r.settings[ZOMBOTANY]?"On":"Off"),
                ModText::Tr("首领：","Boss: ")+std::string(r.settings[MODE]==3?
                    (EndlessRules::BossPeriod(r.settings[ENDLESS])==10?ModText::Tr("每十关","Every 10 stages"):EndlessRules::BossPeriod(r.settings[ENDLESS])==5?ModText::Tr("每五关","Every 5 stages"):ModText::Tr("关闭","Off")):
                    (r.settings[BOSS]==0?ModText::Tr("关闭","Off"):r.settings[BOSS]==1?ModText::Tr("仅末关","Final stage"):ModText::Tr("末关与中途","Final and mid-run")))
            };
            for(size_t i=0;i<lines.size();++i)SeatUi::Text(g,lines[i],64,156+int(i)*32,672,19,Color(220,225,210));
            SeatUi::Button(g,Prev(),ModText::Tr("返回列表","List"),true,down==104);
        } else {
            for(int i=0;i<3;++i)SeatUi::Button(g,Filter(i),i==0?ModText::Tr("全部","All"):i==1?ModText::Tr("闯关","Runs"):ModText::Tr("无尽","Endless"),true,down==110+i,filter==i);
            auto list=Visible();
            for(int i=0;i<5 && page*5+i<int(list.size());++i) {
                const auto& r=records[list[page*5+i]];const auto box=Row(i);SeatUi::Strip(g,box);
                SeatUi::Text(g,Mode(r)+ModText::Tr("  ·  第 ","  ·  Stage ")+std::to_string(r.stage),box.mX+12,box.mY+24,420,20,Color(245,230,165));
                SeatUi::Text(g,Time(r.updated),box.mX+448,box.mY+24,244,18,Color(210,220,225));
                SeatUi::Text(g,Status(r)+std::string("  ·  ")+std::to_string(r.players)+ModText::Tr("人  ·  点击查看详情"," players  ·  Tap for details"),box.mX+12,box.mY+46,680,17,Color(200,210,190));
            }
            if(list.empty())SeatUi::Paragraph(g,readState==RunHistory::DAMAGED?ModText::Tr("记录文件无法读取，原文件已保留，游戏存档不受影响。","History could not be read. Files are retained; game saves are unaffected."):
                ModText::Tr("暂无记录。此版本开始保存最近50局；旧局在继续并保存后加入。","No records yet. This version keeps the latest 50 runs. Older runs appear after resuming and saving."),Rect(72,220,656,130),21,Color(215,225,205));
            SeatUi::Text(g,ModText::Tr("第 ","Page ")+std::to_string(page+1)+" / "+std::to_string(std::max(1,(int(list.size())+4)/5)),48,466,704,18,Color(210,220,195),true);
            SeatUi::Button(g,Prev(),ModText::Tr("上页","Prev"),page>0,down==101);
            SeatUi::Button(g,Next(),ModText::Tr("下页","Next"),(page+1)*5<int(list.size()),down==102);
        }
        SeatUi::Text(g,RunHistory::WriteFailed()?ModText::Tr("最近一次记录保存失败，请检查可用空间。","The latest history write failed. Check available storage."):
            readState==RunHistory::RECOVERED?ModText::Tr("当前显示备份中的记录。","Showing records recovered from backup."):
            ModText::Tr("仅当前玩家的本机记录；记录页不提供读档。","Local records for this profile. This page does not load saves."),48,488,704,17,Color(210,210,190),true);
        SeatUi::Button(g,Back(),ModText::Tr("返回","Back"),true,down==100);
    }
    void MouseDown(int x,int y,int clicks)override{down=clicks>0?Hit(x,y):-1;MarkDirty();}
    void MouseUp(int x,int y,int clicks)override {
        int hit=Hit(x,y),pressed=down;down=-1;if(clicks<=0||hit<0||hit!=pressed)return;
        if(hit==100){app->KillDialog(DIALOG_RUN_HISTORY);return;}
        if(hit==104)selected=-1;else if(hit==101)--page;else if(hit==102)++page;
        else if(hit>=110){filter=hit-110;page=0;}else selected=hit;MarkDirty();
    }
    void KeyDown(KeyCode key)override{if(key==KEYCODE_ESCAPE){if(selected>=0){selected=-1;MarkDirty();}else app->KillDialog(DIALOG_RUN_HISTORY);}}
};
