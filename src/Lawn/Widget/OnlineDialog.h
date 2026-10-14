#pragma once
#include "widget/Dialog.h"
#include "widget/EditListener.h"
#include <memory>
#include <vector>
class LawnApp;
namespace Sexy { class EditWidget; class ButtonWidget; }
class OnlineDialog : public Sexy::Dialog, public Sexy::EditListener
{
public:
    explicit OnlineDialog(LawnApp* app);
    ~OnlineDialog() override;
    void Draw(Sexy::Graphics* g) override;
    void Update() override;
    void ButtonDepress(int id) override;
    void EditWidgetText(int id, const std::string&) override;
    void KeyDown(Sexy::KeyCode key) override;
private:
    LawnApp* mApp;
    std::unique_ptr<Sexy::EditWidget> mAddress, mServer, mCode;
    std::vector<std::unique_ptr<Sexy::ButtonWidget>> mButtons;
    std::string mInputError;
    std::string mLocalIp;
};
