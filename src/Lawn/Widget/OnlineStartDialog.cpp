#include "OnlineStartDialog.h"
#include "../../LawnApp.h"
#include "../../Resources.h"
#include "../ModText.h"
#include "../../PvzpLib/PvzpStringFile.h"
#include "widget/ButtonWidget.h"
#include "graphics/Graphics.h"

OnlineStartDialog::OnlineStartDialog(LawnApp* app, const char* title, const char* body,
    const char* yes, const char* no, Notify notify, bool, int id)
    : Dialog(nullptr, nullptr, id, true, "", "", "", BUTTONS_NONE),
      mApp(app), mNotify(notify), mTitle(title ? title : ""), mBody(body ? body : "")
{
    Resize(0, 0, 660, 310);
    const int count = (yes != nullptr) + (no != nullptr);
    auto add = [&](int buttonId, const char* label, int index) {
        auto b = std::make_unique<Sexy::ButtonWidget>(buttonId, this);
        b->mLabel = label;
        b->SetFont(ModText::FontFor(Sexy::FONT_DWARVENTODCRAFT18, 26));
        b->SetLabelColor(Sexy::Color(245, 230, 165));
        b->SetBkgColor(Sexy::Color(37, 83, 55));
        b->Resize(count == 1 ? 200 : 75 + index * 300, 235, 260, 55);
        AddWidget(b.get());
        mButtons.push_back(std::move(b));
    };
    if (yes) add(ID_YES, yes, 0);
    if (no) add(ID_NO, no, yes ? 1 : 0);
}
OnlineStartDialog::~OnlineStartDialog() { for (auto& b : mButtons) RemoveWidget(b.get()); }
void OnlineStartDialog::SetBody(const char* body) { mBody = body ? body : ""; MarkDirty(); }
void OnlineStartDialog::Draw(Sexy::Graphics* g)
{
    g->SetColor(Sexy::Color(23, 57, 38)); g->FillRect(0, 0, mWidth, mHeight);
    g->SetColor(Sexy::Color(245, 230, 165));
    g->SetFont(ModText::FontFor(Sexy::FONT_DWARVENTODCRAFT24, 34));
    g->DrawString(mTitle, 30, 52);
    PvzpDrawStringWrapped(g, mBody, Sexy::Rect(30, 78, 600, 145),
        ModText::FontFor(Sexy::FONT_DWARVENTODCRAFT18, 24),
        Sexy::Color(245, 230, 165), DrawStringJustification::DS_ALIGN_LEFT);
}
void OnlineStartDialog::ButtonDepress(int id)
{
    // Copy before notification: the app may remove this dialog immediately.
    auto* app = mApp; const auto notify = mNotify;
    if (notify == NOTIFY_INVITE_ANSWER) app->OnlineStartPromptAnswer(id == ID_YES);
    else if (notify == NOTIFY_WAIT_CANCEL) app->OnlineStartWaitCancelled();
    else if (notify == NOTIFY_RECONNECT_CANCEL) app->OnlineReconnectCancelled();
    else Dialog::ButtonDepress(id);
}
void OnlineStartDialog::KeyDown(Sexy::KeyCode key)
{
    if (key == Sexy::KEYCODE_ESCAPE && !mButtons.empty())
        ButtonDepress(mButtons.back()->mId);
}
