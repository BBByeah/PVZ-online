#pragma once
#include "../../Resources.h"
#include "graphics/Graphics.h"
#include "graphics/ImageFont.h"

class MobileButtonFont : public Sexy::_Font
{
    Sexy::ImageFont* mArtwork;

public:
    explicit MobileButtonFont(bool highlighted = false)
        : mArtwork(static_cast<Sexy::ImageFont*>(highlighted ? Sexy::FONT_DWARVENTODCRAFT18BRIGHTGREENINSET : Sexy::FONT_DWARVENTODCRAFT18GREENINSET))
    {
        mAscent = mArtwork->GetAscent();
        mAscentPadding = mArtwork->GetAscentPadding();
        mHeight = mArtwork->GetHeight();
        mLineSpacingOffset = mArtwork->GetLineSpacingOffset();
    }

    int CharWidth(char32_t character) override { return mArtwork->CharWidth(character); }
    int CharWidthKern(char32_t character, char32_t previous) override { return mArtwork->CharWidthKern(character, previous); }
    int StringWidth(std::string_view text) override { return mArtwork->StringWidth(text); }
    void DrawString(Sexy::Graphics* graphics, int x, int y, std::string_view text,
        const Sexy::Color& color, const Sexy::Rect& clip) override
    {
        mArtwork->DrawString(graphics, x, y, text, color, clip);
    }
    Sexy::_Font* Duplicate() override { return new MobileButtonFont(*this); }
};
