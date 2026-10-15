#pragma once
#include <algorithm>
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
    while(size > 16 && font->StringWidth(label) > rect.mWidth - 16)
        font = ModText::FontFor(Sexy::FONT_DWARVENTODCRAFT18, --size);
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
    // @pvz-online: preserve the original frame instead of stretching only its flat center.
    Sexy::Image* tiles[3][3] = {
        {Sexy::IMAGE_DIALOG_TOPLEFT, Sexy::IMAGE_DIALOG_TOPMIDDLE, Sexy::IMAGE_DIALOG_TOPRIGHT},
        {Sexy::IMAGE_DIALOG_CENTERLEFT, Sexy::IMAGE_DIALOG_CENTERMIDDLE, Sexy::IMAGE_DIALOG_CENTERRIGHT},
        {Sexy::IMAGE_DIALOG_BOTTOMLEFT, Sexy::IMAGE_DIALOG_BOTTOMMIDDLE, Sexy::IMAGE_DIALOG_BOTTOMRIGHT}
    };
    int left=std::min(tiles[0][0]->mWidth,rect.mWidth/3);
    int right=std::min(tiles[0][2]->mWidth,rect.mWidth/3);
    int top=std::min(tiles[0][0]->mHeight,rect.mHeight/3);
    int bottom=std::min(tiles[2][0]->mHeight,rect.mHeight/3);
    int widths[]={left,rect.mWidth-left-right,right};
    int heights[]={top,rect.mHeight-top-bottom,bottom};
    Sexy::Graphics background(*g); background.SetColorizeImages(false);
    int y=rect.mY;
    for(int row=0;row<3;++row) {
        int x=rect.mX;
        for(int col=0;col<3;++col) {
            auto* image=tiles[row][col];
            background.DrawImage(image,Sexy::Rect(x,y,widths[col],heights[row]),
                Sexy::Rect(0,0,image->mWidth,image->mHeight));
            x+=widths[col];
        }
        y+=heights[row];
    }
}
