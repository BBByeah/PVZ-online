#include "ZombieVoteDialog.h"
#include "../../LawnApp.h"
#include "../../PvzpLib/PvzpStringFile.h"
#include "../Run/RunState.h"
#include "MobileCoopDraw.h"

ZombieVoteDialog::ZombieVoteDialog(LawnApp *app)
    : LawnDialog(app, DIALOG_ZOMBIE_VOTE, true, "", "", "", BUTTONS_NONE)
{
    Resize(0, 0, 730, 470);
    for (int i = 0; i < 4; ++i)
    {
        auto button = std::make_unique<MobileStoneButton>(i, this);
        button->mLabel =
            i < 3 ? GetZombieBuffDisplayName(app->mZombieVoteCandidates[i]) : ModText::Tr("弃权", "Abstain");
        button->Resize(i < 3 ? 25 + i * 230 : 255, i < 3 ? 165 : 375, 215, 52);
        AddWidget(button.get());
        mChoices.push_back(std::move(button));
    }
}
ZombieVoteDialog::~ZombieVoteDialog()
{
    for (auto &b : mChoices)
        RemoveWidget(b.get());
}
void ZombieVoteDialog::Update()
{
    LawnDialog::Update();
    for (auto &b : mChoices)
        b->SetDisabled(mApp->mZombieVoteMyChoice >= 0);
    MarkDirty();
}
void ZombieVoteDialog::Draw(Sexy::Graphics *g)
{
    LawnDialog::Draw(g);
    g->SetColor(Sexy::Color(245, 230, 165));
    g->SetFont(ModText::FontFor(Sexy::FONT_DWARVENTODCRAFT24, 30));
    g->DrawString(ModText::Tr("僵尸强化投票", "Zombie boost vote"), 220, 87);
    g->SetFont(ModText::FontFor(Sexy::FONT_DWARVENTODCRAFT18, 22));
    g->DrawString(mApp->ZombieVoteStatusText(), 175, 125);
    for (int i = 0; i < 3; ++i)
    {
        const int id = mApp->mZombieVoteCandidates[i];
        PvzpDrawStringWrapped(g, GetZombieBuffDesc(id), Sexy::Rect(32 + i * 230, 234, 202, 105),
                              ModText::FontFor(Sexy::FONT_DWARVENTODCRAFT18, 21), Sexy::Color(245, 230, 165),
                              DrawStringJustification::DS_ALIGN_LEFT);
        g->SetFont(ModText::FontFor(Sexy::FONT_DWARVENTODCRAFT18, 20));
        g->DrawString(ModText::Format("已有 {}", "Owned {}", mApp->mRunState->ZombieBuffCount(id)),
                      50 + i * 230, 350);
    }
}
void ZombieVoteDialog::ButtonDepress(int id)
{
    if (id < 0 || id > 3 || mApp->mZombieVoteMyChoice >= 0)
        return;
    for (auto &b : mChoices)
        b->SetDisabled(true);
    mApp->ZombieVoteChosen(id);
}
