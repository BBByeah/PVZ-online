#pragma once
#include "../../Resources.h"
#include "../ModText.h"
#include "GameButton.h"
#include "graphics/Font.h"
#include "graphics/Graphics.h"
#include "graphics/Image.h"
#include "widget/ButtonWidget.h"

// Native stone art with a separate UTF-8 label, shared by touch and vote UI.
inline void DrawMobileStone(Sexy::Graphics *g, const Sexy::Rect &rect, const std::string &label,
                            bool down = false, bool enabled = true, int size = 22)
{
    Sexy::Graphics stone(*g);
    stone.SetColorizeImages(false);
    auto* left = down ? Sexy::IMAGE_BUTTON_DOWN_LEFT : Sexy::IMAGE_BUTTON_LEFT;
    auto* middle = down ? Sexy::IMAGE_BUTTON_DOWN_MIDDLE : Sexy::IMAGE_BUTTON_MIDDLE;
    auto* right = down ? Sexy::IMAGE_BUTTON_DOWN_RIGHT : Sexy::IMAGE_BUTTON_RIGHT;
    stone.DrawImage(left, Sexy::Rect(rect.mX, rect.mY, left->mWidth, rect.mHeight),
        Sexy::Rect(0, 0, left->mWidth, left->mHeight));
    stone.DrawImage(middle, Sexy::Rect(rect.mX + left->mWidth, rect.mY,
        rect.mWidth - left->mWidth - right->mWidth, rect.mHeight),
        Sexy::Rect(0, 0, middle->mWidth, middle->mHeight));
    stone.DrawImage(right, Sexy::Rect(rect.mX + rect.mWidth - right->mWidth, rect.mY,
        right->mWidth, rect.mHeight), Sexy::Rect(0, 0, right->mWidth, right->mHeight));
    auto *font = ModText::FontFor(Sexy::FONT_DWARVENTODCRAFT18, size);
    g->SetFont(font);
    g->SetColor(enabled ? Sexy::Color(30, 75, 15) : Sexy::Color(115, 115, 115));
    g->DrawString(label, rect.mX + (rect.mWidth - font->StringWidth(label)) / 2,
                  rect.mY + (rect.mHeight + font->GetAscent()) / 2 - 3);
}

class MobileStoneButton : public Sexy::ButtonWidget
{
  public:
    MobileStoneButton(int id, Sexy::ButtonListener *listener) : ButtonWidget(id, listener)
    {
    }
    void Draw(Sexy::Graphics *g) override
    {
        DrawMobileStone(g, Sexy::Rect(0, 0, mWidth, mHeight), mLabel, mIsDown && mIsOver, !mDisabled);
    }
};

inline void DrawMobilePanel(Sexy::Graphics *g, const Sexy::Rect &rect)
{
    auto *image = Sexy::IMAGE_DIALOG_CENTERMIDDLE;
    Sexy::Graphics background(*g);
    background.SetColorizeImages(false);
    background.DrawImage(image, rect, Sexy::Rect(0, 0, image->mWidth, image->mHeight));
    g->SetColor(Sexy::Color(70, 65, 65));
    g->DrawRect(rect);
}
