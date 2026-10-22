#pragma once
#include "widget/Dialog.h"
#include "../../ConstEnums.h"
#include <memory>
#include <vector>
class LawnApp;
namespace Sexy { class ButtonWidget; }

// Keeps the PC notification interface; native UTF-8 fonts handle both languages.
class OnlineStartDialog : public Sexy::Dialog
{
public:
    enum Notify { NOTIFY_NONE, NOTIFY_INVITE_ANSWER, NOTIFY_WAIT_CANCEL, NOTIFY_RECONNECT_CANCEL };
    OnlineStartDialog(LawnApp* app, const char* title, const char* body,
        const char* yes, const char* no, Notify notify, bool draggable = false,
        int id = DIALOG_ONLINE_START);
    ~OnlineStartDialog() override;
    void SetBody(const char* body);
    void Draw(Sexy::Graphics* g) override;
    void ButtonDepress(int id) override;
    void KeyDown(Sexy::KeyCode key) override;
private:
    LawnApp* mApp;
    Notify mNotify;
    std::string mTitle, mBody;
    std::vector<std::string> mPages;
    size_t mPage = 0;
    std::vector<std::unique_ptr<Sexy::ButtonWidget>> mButtons;
};
