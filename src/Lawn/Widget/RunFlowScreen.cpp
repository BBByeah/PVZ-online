#include "../CoopPlay.h"
#include "MobileCoopDraw.h"
#include "RunFlowScreen.h"
#include "../../LawnApp.h"
#include "../../Resources.h"
#include "../Run/RunState.h"
#include "../Run/PortVersion.h"
#include "../Run/RunBuffs.h"
#include "../Run/RunPortSupport.h"
#include "../ModText.h"
#include "../System/PlayerInfo.h"
#include "../SeedPacket.h"
#include "../../PvzpLib/PvzpCommon.h"
#include "widget/ButtonWidget.h"
#include "graphics/Graphics.h"
#include "graphics/Font.h"
#include "../../PvzpLib/PvzpStringFile.h"
#include <format>

namespace {
// Match PC RunModeDialog's challenge cards and icon IDs, with the same click-to-start flow.
class RunModeCard : public Sexy::ButtonWidget {
    int icon;
public:
    RunModeCard(int id, Sexy::ButtonListener* listener, int cell) : ButtonWidget(id,listener),icon(cell) {}
    void Draw(Sexy::Graphics* g) override {
        int offset=mIsDown&&mIsOver?1:0;
        g->DrawImageCel(Sexy::IMAGE_CHALLENGE_THUMBNAILS,13+offset,4+offset,icon);
        g->DrawImage(mIsOver?Sexy::IMAGE_CHALLENGE_WINDOW:Sexy::IMAGE_CHALLENGE_WINDOW_HIGHLIGHT,-6+offset,-2+offset);
        auto* font=ModText::FontFor(Sexy::FONT_DWARVENTODCRAFT18,18);
        g->SetFont(font);g->SetColor(Sexy::Color(42,42,90));
        g->DrawString(MobileTextLine(font,mLabel,104),offset+(104-font->StringWidth(mLabel))/2,100+offset);
    }
};

// PC rewards group the seed image, description and owned count in each column.
class RunChoiceCard : public Sexy::ButtonWidget {
    LawnApp* app;
public:
    RunChoiceCard(int id, Sexy::ButtonListener* listener, LawnApp* a) : ButtonWidget(id,listener),app(a) {}
    void Draw(Sexy::Graphics* g) override {
        auto& run=*app->mRunState;
        bool plantPick=run.IsPlantPick();
        int choice=plantPick?static_cast<int>(run.mPlantChoices[mId]):run.mBuffChoices[mId];
        if(mDisabled){DrawMobileStone(g,Sexy::Rect(0,274,mWidth,48),mLabel,false,false);return;}
        auto* font=ModText::FontFor(Sexy::FONT_DWARVENTODCRAFT18,20);
        if(!plantPick){
            const char* plantName=GetRunChoicePlantName(choice);
            std::string head=plantName?std::string(plantName):ModText::Tr("全局增益","Global bonus");
            int rarity=GetRunChoiceRarity(choice);
            for(int i=0;i<rarity;++i)head+=" ★";
            PvzpDrawStringWrapped(g,head,Sexy::Rect(4,0,mWidth-8,55),font,
                rarity>=3?Sexy::Color(255,200,60):Sexy::Color(205,205,190),DrawStringJustification::DS_ALIGN_CENTER);
            std::string desc=GetRunChoiceDesc(choice);
            Sexy::Rect descRect(6,60,mWidth-12,170);
            if(PvzpDrawStringWrappedHelper(g,desc,descRect,font,Sexy::Color::White,DS_ALIGN_LEFT,false)>descRect.mHeight)
                font=ModText::FontFor(Sexy::FONT_DWARVENTODCRAFT18,18);
            PvzpDrawStringWrapped(g,desc,descRect,font,Sexy::Color(245,230,165),DrawStringJustification::DS_ALIGN_LEFT);
            std::string owned=ModText::Format("已有 {}","Owned {}",run.GetBuffCount(choice));
            int cap=GetRunChoiceMaxStacks(choice);
            if(cap>0)owned+=" / "+std::to_string(cap);
            PvzpDrawStringWrapped(g,owned,Sexy::Rect(4,238,mWidth-8,30),ModText::FontFor(Sexy::FONT_DWARVENTODCRAFT18,20),Sexy::Color(205,205,190),DrawStringJustification::DS_ALIGN_CENTER);
        } else {
            DrawSeedPacket(g,(mWidth-SEED_PACKET_WIDTH)/2,8,(SeedType)choice,SEED_NONE,0,255,false,false);
            PvzpDrawStringWrapped(g,ModText::Tr("加入本局植物池","Add to this run's collection"),Sexy::Rect(8,126,mWidth-16,80),font,Sexy::Color(205,205,190),DrawStringJustification::DS_ALIGN_CENTER);
        }
        DrawMobileStone(g,Sexy::Rect(0,274,mWidth,48),mLabel,mIsDown&&mIsOver,true,22);
    }
};
}

RunFlowScreen::RunFlowScreen(LawnApp* app, bool modePage) : mApp(app), mModePage(modePage)
{
    Resize(0, 0, 800, 600);
    auto addButton = [&](int id, const std::string& text, int x, int y, int width, int height) {
        auto* button = new MobileStoneButton(id, this);
        button->mLabel = text;
        button->SetFont(ModText::FontFor(Sexy::FONT_DWARVENTODCRAFT18, 28));
        button->SetLabelColor(Sexy::Color(245, 230, 165));
        button->SetLabelHiliteColor(Sexy::Color::White);
        button->SetBkgColor(Sexy::Color(37, 83, 55));
        button->Resize(x, y, width, height);
        AddWidget(button);
        if(mModePage)mModeButtons.push_back(button);
        return button;
    };
    if(!mModePage && app->IsOnlineGame())addButton(30,ModText::Tr("协作 / 借用 B","Teamwork B"),570,65,190,42);
    if (mModePage)
    {
        PvzpLoadResources("DelayLoad_ChallengeScreen");
        const int icons[]={19,16,18};
        for(int i=0;i<3;++i){
            auto* card=new RunModeCard(10+i,this,icons[i]);
            const char* labels[]={ModText::Tr("完整25关","Full 25"),ModText::Tr("普通10关","Normal 10"),ModText::Tr("快速5关","Quick 5")};
            card->mLabel=labels[i];card->Resize(174+i*174,132,104,115);AddWidget(card);mModeButtons.push_back(card);
        }
        addButton(15, ModText::Tr("无尽", "Endless"), 300, 266, 200, 48);
        for(int i=0;i<3;++i)addButton(40+i,ModText::Tr(i==0?"轻松 ×0.5":i==1?"标准 ×1":"高压 ×2",i==0?"Easy x0.5":i==1?"Standard x1":"High x2"),122+i*190,366,176,48);
        for (int i = 0; i < 6; ++i)
            mOptionButtons[i] = addButton(20 + i, "", 64 + (i % 2) * 344, 154 + (i / 2) * 74, 328, 54);
        RefreshOptionLabels();
        mZombieBuffButton = addButton(27, ModText::Tr("僵尸强化：关", "Zombie boosts: off"), 64, 376, 328, 54);
        const char* ladders[] = { "1.33", "1", "1.2", "1.5", "2" };
        mLadderButton = addButton(26, ModText::Format("场景阶梯：×{}", "Scene ladder: x{}", ladders[mLadder]), 408, 376, 328, 54);
        mBuffMultButton = addButton(14, ModText::Format("增益倍选：×{}", "Buff picks: x{}", RunState::BuffMultValueFor(mBuffMult)), 64, 450, 328, 44);
        addButton(13, ModText::Tr("继续闯关", "Continue run"), 310, 510, 180, 48)->SetDisabled(!RunState::HasCheckpoint(app->mPlayerInfo->mId));
        addButton(28,ModText::Tr("高级选项", "Advanced"),90,510,180,48);
    }
    else
    {
        RunState& run = *app->mRunState;
        if (!run.ChoicesRolled()) run.RollChoices();
        for (int i = 0; i < RunState::RUN_CHOICES; ++i)
        {
            const bool plant = run.IsPlantPick();
            const bool valid = plant ? run.mPlantChoices[i] != SEED_NONE : run.mBuffChoices[i] != RunState::RUN_BUFF_CHOICE_NONE;
            const char* label = !valid ? ModText::Tr("不可用", "Unavailable") : plant ? GetRunPlantName(run.mPlantChoices[i]) : GetRunChoiceDisplayName(run.mBuffChoices[i]);
            auto* card=new RunChoiceCard(i,this,app);card->mLabel=label?label:"Plant";
            card->Resize(56+i*240,178,208,322);card->SetDisabled(!valid);AddWidget(card);
        }
        const int cost = run.GetPaidRerollCost();
        addButton(4, (run.mRerollVouchers > 0 ? ModText::Format("重摇券 ×{}", "Reroll voucher x{}", run.mRerollVouchers) : ModText::Format("重摇 -{}", "Reroll -{}", LawnApp::GetMoneyString(cost))), 62, 518, 208, 48)
            ->SetDisabled(run.mRerollVouchers == 0 && app->mPlayerInfo->mCoins < cost);
        addButton(5, ModText::Tr("跳过", "Skip"), 530, 518, 208, 48);
    }
    addButton(9, ModText::Tr("返回主菜单", "Back to menu"), mModePage?530:296,mModePage?510:518,208,48);
    if(mModePage)RefreshModePage();
}

void RunFlowScreen::RefreshModePage()
{
    for(auto* b:mModeButtons){
        bool advanced=(b->mId>=20&&b->mId<=27)||b->mId==14;
        b->SetVisible(b->mId==9||b->mId==28||(advanced==mAdvanced));
        if(b->mId>=40&&b->mId<=42)b->mInverted=(b->mId-40==mDifficulty);
        if(b->mId==28)b->mLabel=mAdvanced?ModText::Tr("返回模式","Back"):ModText::Tr("高级选项","Advanced");
    }
    MarkDirty();
}

RunFlowScreen::~RunFlowScreen() { RemoveAllWidgets(true); }

void RunFlowScreen::RefreshOptionLabels()
{
    const char* difficulties[] = { ModText::Tr("轻松", "Easy"), ModText::Tr("标准", "Standard"), ModText::Tr("高压", "High") };
    const char* scales[] = { ModText::Tr("少量", "Half"), ModText::Tr("标准", "Standard"), ModText::Tr("多量", "Double"), ModText::Tr("海量", "Quadruple") };
    const char* tempos[] = { ModText::Tr("快", "Fast"), ModText::Tr("标准", "Standard"), ModText::Tr("慢", "Slow") };
    const char* scenes[] = { ModText::Tr("白天", "Day"), ModText::Tr("夜晚", "Night"), ModText::Tr("泳池", "Pool"), ModText::Tr("迷雾", "Fog"), ModText::Tr("屋顶", "Roof") };
    mOptionButtons[0]->mLabel = ModText::Format("难度：{}", "Difficulty: {}", difficulties[mDifficulty]);
    mOptionButtons[1]->mLabel = ModText::Format("规模：{}", "Scale: {}", scales[mScale]);
    mOptionButtons[2]->mLabel = ModText::Format("节奏：{}", "Tempo: {}", tempos[mTempo]);
    mOptionButtons[3]->mLabel = ModText::Format("植物僵尸：{}", "Zombotany: {}", mZombotany ? ModText::Tr("开", "On") : ModText::Tr("关", "Off"));
    mOptionButtons[4]->mLabel = ModText::Format("关底首领：{}", "Final boss: {}", mBossFlag ? ModText::Tr("开", "On") : ModText::Tr("关", "Off"));
    mOptionButtons[5]->mLabel = ModText::Format("无尽场景：{}", "Endless scene: {}", scenes[mEndlessScene]);
    MarkDirty();
}

void RunFlowScreen::Draw(Sexy::Graphics* g)
{
    DrawMobilePanel(g,Sexy::Rect(20,20,760,560));
    auto* titleFont=ModText::FontFor(Sexy::FONT_DWARVENTODCRAFT24,30);
    std::string title=mModePage?(mAdvanced?ModText::Tr("高级选项","Advanced Options"):ModText::Tr("闯关模式","Roguelike Run")):
        mApp->mRunState->IsPlantPick()?ModText::Tr("选择植物","Choose a plant"):ModText::Tr("选择增益","Choose a bonus");
    g->SetFont(titleFont);g->SetColor(Sexy::Color(245,230,165));
    g->DrawString(title,(800-titleFont->StringWidth(title))/2,90);
    auto* font=ModText::FontFor(Sexy::FONT_DWARVENTODCRAFT18,20);
    if(mModePage){
        auto text=mAdvanced?ModText::Tr("设置完成后，返回模式选择关卡。","Return to the mode page to start after setting options."):
            ModText::Tr("点击卡片开始；开局前可调整难度与高级选项。","Choose a card to start; adjust difficulty and options first.");
        PvzpDrawStringWrapped(g,text,Sexy::Rect(80,104,640,46),font,Sexy::Color(205,205,190),DrawStringJustification::DS_ALIGN_CENTER);
        if(!mAdvanced)PvzpDrawStringWrapped(g,ModText::Tr("出怪难度","Zombie difficulty"),Sexy::Rect(150,328,500,28),font,Sexy::Color(205,205,190),DrawStringJustification::DS_ALIGN_CENTER);
        else PvzpDrawStringWrapped(g,ModText::Tr("无尽使用固定场景；场景阶梯仅影响有限关卡。","Endless uses its chosen scene; the ladder affects finite runs."),Sexy::Rect(408,446,328,50),font,Sexy::Color(205,205,190),DrawStringJustification::DS_ALIGN_LEFT);
    }else{
        const auto& run=*mApp->mRunState;
        auto state=run.IsEndless()?ModText::Format("无尽第 {} 关","Endless level {}",run.mLevelIndex+1):ModText::Format("第 {} / {} 关","Level {} / {}",run.mLevelIndex+1,run.GetLevelCount());
        g->DrawImage(Sexy::IMAGE_COINBANK,64,118);
        g->SetFont(Sexy::FONT_CONTINUUMBOLD14);g->SetColor(Sexy::Color(180,255,90));
        auto money=LawnApp::GetMoneyString(mApp->mPlayerInfo->mCoins);
        g->DrawString(money,180-Sexy::FONT_CONTINUUMBOLD14->StringWidth(money),142);
        PvzpDrawStringWrapped(g,state,Sexy::Rect(64,110,672,52),font,Sexy::Color(205,205,190),DrawStringJustification::DS_ALIGN_CENTER);
    }
}

void RunFlowScreen::ButtonDepress(int id)
{
    if(id==30){mApp->ShowCoopPlay();return;}
    if(mApp->mCoopPlay && mApp->mCoopPlay->Blocking())return;
    if (id == 9) {
        if (!mModePage && mApp->mRunState) {
            if (!mApp->mRunState->SaveWithPlayer(*mApp->mPlayerInfo)) return;
            if (mApp->mCoopPlay) mApp->mCoopPlay->Close();
            mApp->DoBackToMain(true);
        } else mApp->ShowGameSelector();
        return;
    }
    if (mModePage)
    {
        if(id==28){mAdvanced=!mAdvanced;RefreshModePage();return;}
        if(id>=40&&id<=42){mDifficulty=id-40;RefreshOptionLabels();RefreshModePage();return;}
        if (id >= 20 && id <= 25)
        {
            if (id == 20) mDifficulty = (mDifficulty + 1) % 3;
            if (id == 21) mScale = (mScale + 1) % 4;
            if (id == 22) mTempo = (mTempo + 1) % 3;
            if (id == 23) mZombotany ^= 1;
            if (id == 24) mBossFlag ^= 1;
            if (id == 25) mEndlessScene = (mEndlessScene + 1) % 5;
            RefreshOptionLabels();
            RefreshModePage();
            return;
        }
        if (id == 27)
        {
            mZombieBuff = (mZombieBuff + 1) % 3;
            mZombieBuffButton->mLabel = ModText::Format("僵尸强化：{}", "Zombie boosts: {}", mZombieBuff ? std::to_string(mZombieBuff) : ModText::Tr("关", "off"));
            MarkDirty(); return;
        }
        if (id == 26)
        {
            mLadder = (mLadder + 1) % 5;
            const char* ladders[] = { "1.33", "1", "1.2", "1.5", "2" };
            mLadderButton->mLabel = ModText::Format("场景阶梯：×{}", "Scene ladder: x{}", ladders[mLadder]);
            MarkDirty();
            return;
        }
        if (id == 14)
        {
            mBuffMult = (mBuffMult + 1) % 3;
            mBuffMultButton->mLabel = ModText::Format("增益倍选：×{}", "Buff picks: x{}", RunState::BuffMultValueFor(mBuffMult));
            MarkDirty();
            return;
        }
        if (id == 13) mApp->ResumeRun();
        else if ((id >= 10 && id <= 12) || id == 15)
            mApp->StartRun(id == 15 ? RunState::RUN_MODE_ENDLESS : id - 10, mBuffMult,
                mDifficulty, mScale, mTempo, mZombotany, mEndlessScene, mBossFlag, mLadder, mZombieBuff);
        return;
    }
    RunState& run = *mApp->mRunState;
    if (id == 4) mApp->RunPickPaidReroll();
    else if (id == 5) run.SkipPendingPick();
    else if (id >= 0 && id < RunState::RUN_CHOICES)
    {
        if (run.IsPlantPick()) run.TakePlantChoice(id);
        else {
            int choice=run.mBuffChoices[id], before=run.GetEffectiveBuffCount(choice);
            run.TakeBuffChoice(id);
            mApp->RefreshRunChoicePlants(choice,before);
        }
    }
    mApp->ShowRunPicks();
}
