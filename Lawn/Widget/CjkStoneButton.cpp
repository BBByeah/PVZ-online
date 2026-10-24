#include "CjkStoneButton.h"
#include "../ModText.h"
#include "../../LawnApp.h"
#include "../../Resources.h"
#include "graphics/Graphics.h"
#include "graphics/ImageFont.h"
#include "ArtworkButtonFont.h"

int CjkPointSize(int thePixelHeight)
{
	HDC aDC = ::GetDC(gSexyAppBase->mHWnd);
	int aDpi = GetDeviceCaps(aDC, LOGPIXELSY);
	::ReleaseDC(gSexyAppBase->mHWnd, aDC);
	if (aDpi <= 0) aDpi = 96;
	int aPointSize = (thePixelHeight * 72 + aDpi / 2) / aDpi;
	return aPointSize < 1 ? 1 : aPointSize;
}

int CjkStoneButtonWidth(int theWidth, bool theRoundUp)
{
	int aMid = Sexy::IMAGE_BUTTON_MIDDLE->mWidth;
	int aMin = Sexy::IMAGE_BUTTON_LEFT->mWidth + Sexy::IMAGE_BUTTON_RIGHT->mWidth;
	int anExtra = theWidth - aMin;
	if (anExtra < 0)
	{
		anExtra = 0;
	}
	else if (aMid > 0)
	{
		int aRemainder = anExtra % aMid;
		if (aRemainder != 0)
		{
			if (theRoundUp) anExtra += aMid - aRemainder;
			else anExtra -= aRemainder;
		}
	}
	int aWidth = aMin + anExtra;
	int aFloor = aMin + aMid;   // 至少带一个中段：光两块端头拼不成石头
	return aWidth < aFloor ? aFloor : aWidth;
}

namespace {
bool HasArtwork(Sexy::ImageFont* font, wchar_t character)
{
	font->Prepare();
	if (character == L' ') return true;
	if (character < 128) character = static_cast<unsigned char>(font->GetMappedChar(static_cast<SexyChar>(character)));
	for (auto& layer : font->mActiveLayerList) {
		auto glyph = layer.mBaseFontLayer->mCharDataMap.find(character);
		if (glyph != layer.mBaseFontLayer->mCharDataMap.end() && glyph->second.mWidth > 0) return true;
	}
	return false;
}
// Keep complete runs of existing artwork, including their original kerning.
// Only genuinely absent glyphs use the fallback, never the whole label.
template<class Consumer> void LabelRuns(const std::string& text, Consumer consume)
{
	auto* font = static_cast<Sexy::ImageFont*>(Sexy::FONT_DWARVENTODCRAFT18GREENINSET);
	int offset = 0, start = 0;
	bool artwork = true;
	while (offset < static_cast<int>(text.size())) {
		int before = offset;
		wchar_t character = Sexy::UTF8NextChar(text, offset);
		bool current = HasArtwork(font, character);
		if (before > start && current != artwork) { consume(text.substr(start, before-start), artwork); start = before; }
		artwork = current;
	}
	if (offset > start) consume(text.substr(start), artwork);
}
ModText::Font* MissingGlyphFont()
{
	return ModText::GetFont(CjkPointSize(14), true);
}
int FallbackWidth(const std::wstring& text)
{
    const int cell = Sexy::FONT_DWARVENTODCRAFT18GREENINSET->CharWidth(L'房');
    return std::max(ModText::TextWidth(MissingGlyphFont(), text), cell * static_cast<int>(text.size()));
}
}

bool CjkLabelHasArtwork(const std::string& theLabel)
{
	int offset = 0;
	auto* font = static_cast<Sexy::ImageFont*>(Sexy::FONT_DWARVENTODCRAFT18GREENINSET);
	while (offset < static_cast<int>(theLabel.size()))
	{
		wchar_t character = Sexy::UTF8NextChar(theLabel, offset);
		if (!HasArtwork(font, character)) return false;
	}
	return true;
}

int CjkStoneButtonLabelWidth(const std::string& theLabel)
{
	int width = 0;
	LabelRuns(theLabel, [&](const std::string& run, bool artwork) {
		width += artwork ? Sexy::FONT_DWARVENTODCRAFT18GREENINSET->StringWidth(run) :
			FallbackWidth(ModText::WideFromUtf8(run.c_str()));
	});
	return width;
}

void DrawCjkStoneLabel(Graphics* g, const std::string& label, int x, int baseline, bool highlighted, bool enabled)
{
	ArtworkButtonFont artwork(highlighted);
	LabelRuns(label, [&](const std::string& run, bool useArtwork) {
		if (useArtwork) {
			artwork.DrawString(g, x, baseline, run, enabled ? Color::White : Color(145,145,145), g->mClipRect);
			x += artwork.StringWidth(run);
		} else {
			auto* font = MissingGlyphFont();
			auto wide = ModText::WideFromUtf8(run.c_str());
			int y = baseline - ModText::Ascent(font) + 2;
            int cellX = x + (FallbackWidth(wide) - ModText::TextWidth(font,wide)) / 2;
			static const int outline[4][2] = {{1,0},{-1,0},{0,1},{0,-1}};
			for (auto& delta : outline) ModText::DrawTextWide(g,font,cellX+delta[0],y+delta[1],wide,Color(16,44,16),g->mClipRect);
			ModText::DrawTextWide(g,font,cellX,y,wide,!enabled ? Color(145,145,145) : highlighted ? Color(155,240,96) : Color(0,196,0),g->mClipRect);
			x += FallbackWidth(wide);
		}
	});
}

void CjkStoneButton::Draw(Graphics* g)
{
	if (mBtnNoDraw) return;

	if (CjkLabelHasArtwork(mLabel))
	{
		LawnStoneButton::Draw(g);
		return;
	}

	bool aDown = (mIsDown && mIsOver && !mDisabled) ^ mInverted;
	Image* aLeftImage = aDown ? Sexy::IMAGE_BUTTON_DOWN_LEFT : Sexy::IMAGE_BUTTON_LEFT;
	Image* aMiddleImage = aDown ? Sexy::IMAGE_BUTTON_DOWN_MIDDLE : Sexy::IMAGE_BUTTON_MIDDLE;
	Image* aRightImage = aDown ? Sexy::IMAGE_BUTTON_DOWN_RIGHT : Sexy::IMAGE_BUTTON_RIGHT;

	int aFontX = 0;
	int aFontY = 0;
	int aImageX = 0;
	if (aDown)
	{
		aFontX++;
		aFontY++;
		aImageX++;
	}

	// 石头铺满控件宽（2026-10-14 出框修复，同 GameButton.cpp DrawStoneButton）
	int aRepeat = (mWidth - aLeftImage->mWidth - aRightImage->mWidth + aMiddleImage->mWidth - 1) / aMiddleImage->mWidth;
	if (aRepeat < 0) aRepeat = 0;
	g->DrawImage(aLeftImage, aImageX, 0);
	aImageX += aLeftImage->mWidth;
	int aRightX = aImageX + mWidth - aLeftImage->mWidth - aRightImage->mWidth;
	while (aRepeat > 0)
	{
		if (aImageX + aMiddleImage->mWidth <= aRightX)
			g->DrawImage(aMiddleImage, aImageX, 0);
		else
			g->DrawImage(aMiddleImage, aImageX, 0, Rect(0, 0, aRightX - aImageX, aMiddleImage->mHeight));
		aImageX += aMiddleImage->mWidth;
		--aRepeat;
	}
	g->DrawImage(aRightImage, aRightX, 0);

	aFontX += (mWidth - CjkStoneButtonLabelWidth(mLabel)) / 2;
	aFontY += (mHeight + Sexy::FONT_DWARVENTODCRAFT18GREENINSET->GetAscent()) / 2 - 3;
	Graphics text(*g);
	text.ClipRect(0, 0, mWidth, mHeight);
	DrawCjkStoneLabel(&text,mLabel,aFontX,aFontY,aDown || mIsOver,!mDisabled);
}
