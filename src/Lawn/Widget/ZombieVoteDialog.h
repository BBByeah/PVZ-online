#pragma once
#include "LawnDialog.h"
#include <memory>
#include <vector>
class MobileStoneButton;
class ZombieVoteDialog : public LawnDialog
{
  public:
    explicit ZombieVoteDialog(LawnApp *app);
    ~ZombieVoteDialog() override;
    void Update() override;
    void Draw(Sexy::Graphics *g) override;
    void ButtonDepress(int id) override;
    void KeyDown(Sexy::KeyCode) override
    {
    }

  private:
    std::vector<std::unique_ptr<MobileStoneButton>> mChoices;
};
