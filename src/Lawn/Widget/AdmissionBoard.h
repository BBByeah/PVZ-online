#pragma once
#include "../../LawnApp.h"
#include "../Board.h"
#include "../SeedPacket.h"
#include "../CursorObject.h"
#include "../Run/RunState.h"
#include "OnlineUiDraw.h"
#include "GameButton.h"
#include "widget/WidgetManager.h"

namespace AdmissionBoard {
using namespace Sexy;
inline Rect Finish(){return Rect(590,542,186,48);}
inline bool Instant(SeedType seed){return seed==SEED_CHERRYBOMB || seed==SEED_JALAPENO || seed==SEED_DOOMSHROOM || seed==SEED_ICESHROOM || seed==SEED_GRAVEBUSTER;}
inline bool Update(Board* b) {
    auto* app=b->mApp;if(!app->AdmissionBuilding())return false;
    b->Widget::Update();b->MarkDirty();
    b->mMenuButton->mBtnNoDraw=false;b->mMenuButton->mDisabled=!b->CanInteractWithBoardButtons();b->mMenuButton->Update();
    b->UpdateMousePosition();
    b->mCursorPreview->Update();b->mCursorObject->Update();
    b->mSeedBank->mVisible=true;b->mShowShovel=true;
    for(int i=0;i<b->mSeedBank->mNumPackets;++i) {
        auto& card=b->mSeedBank->mSeedPackets[i];card.mRefreshing=false;card.mRefreshCounter=0;card.mActive=true;
    }
    b->ProcessDeleteQueue();return true;
}
inline bool MouseDown(Board* b,int x,int y,int clicks) {
    auto* app=b->mApp;if(!app->AdmissionBuilding())return false;
    if(clicks>0 && Finish().Contains(x,y)) {app->mAdmissionPressed=true;b->mIgnoreMouseUp=true;return true;}
    app->mAdmissionDirty=true;
    return false;
}
inline bool MouseUp(Board* b,int x,int y,int clicks) {
    auto* app=b->mApp;if(!app->AdmissionBuilding())return false;
    if(!app->mAdmissionPressed){app->mAdmissionDirty=true;return false;}
    app->mAdmissionPressed=false;b->mIgnoreMouseUp=false;
    if(clicks>0 && Finish().Contains(x,y))app->FinishAdmissionBuilding();return true;
}
inline void Draw(Board* b,Graphics* g) {
    auto* app=b->mApp;auto* run=app->GetRunState();if(!run)return;
    if(run->mAdmission.fast && run->mAdmission.phase==Admission::PICKS) {
        SeatUi::Strip(g,Rect(130,248,540,100));
        SeatUi::Text(g,ModText::Tr("正在快速补齐增益","Catching up buffs"),154,282,492,24,Color(245,230,165));
        SeatUi::Text(g,ModText::Tr("剩余选择：","Choices remaining: ")+std::to_string(run->mPendingBuffPicks),154,322,492,20,Color(220,225,210));return;
    }
    if(!app->AdmissionBuilding())return;
    SeatUi::Strip(g,Rect(24,542,552,54));
    SeatUi::Text(g,ModText::Tr("补位布阵 · 额度 ","Build allowance: ")+std::to_string(run->mAdmission.budget),38,564,520,22,Color(245,230,165));
    SeatUi::Text(g,ModText::Tr("布阵不产资源；完成后剩余额度清零。","No production; unused allowance expires on completion."),38,588,520,16,Color(220,225,210));
    SeatUi::Button(g,Finish(),ModText::Tr("布阵完成","Finish building"),!app->mAdmissionDirty && app->IsOnlineGame() && !b->mPaused,app->mAdmissionPressed);
}
}
