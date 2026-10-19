#pragma once
#include "CjkStoneButton.h"
#include "../ModText.h"

namespace RunChoiceLayout {
inline ModText::Font* DescriptionFont(const char* text, int width, int height)
{
    for (int size = 20; size >= 16; size -= 2)
    {
        auto* font = ModText::GetFont(CjkPointSize(size), false);
        if (ModText::WrapLineCount(font, text, width) * (ModText::LineHeight(font) + 3) <= height || size == 16)
            return font;
    }
    return ModText::GetFont(CjkPointSize(16), false);
}
}
