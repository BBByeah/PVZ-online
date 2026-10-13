#pragma once
#include "widget/Widget.h"
#include "widget/ButtonListener.h"

class LawnApp;
namespace Sexy { class ButtonWidget; }
class RunFlowScreen : public Sexy::Widget, public Sexy::ButtonListener
{
    LawnApp* mApp;
    bool mModePage;
    int mBuffMult = 0;
    Sexy::ButtonWidget* mBuffMultButton = nullptr;
public:
    explicit RunFlowScreen(LawnApp* app, bool modePage);
    ~RunFlowScreen() override;
    void Draw(Sexy::Graphics* g) override;
    void ButtonDepress(int id) override;
    void ButtonPress(int) override {}
    void ButtonDownTick(int) override {}
    void ButtonMouseEnter(int) override {}
    void ButtonMouseLeave(int) override {}
    void ButtonMouseMove(int, int, int) override {}
};
