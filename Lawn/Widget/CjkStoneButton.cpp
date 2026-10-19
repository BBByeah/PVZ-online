#include "CjkStoneButton.h"
#include "../ModText.h"
#include "../../LawnApp.h"
#include "../../Resources.h"
#include "graphics/Graphics.h"
#include "graphics/ImageFont.h"

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

static bool LabelHasArtwork(const std::string& theLabel)
{
	auto* font = static_cast<Sexy::ImageFont*>(Sexy::FONT_DWARVENTODCRAFT18GREENINSET);
	font->Prepare();
	int offset = 0;
	while (offset < static_cast<int>(theLabel.size()))
	{
		wchar_t character = Sexy::UTF8NextChar(theLabel, offset);
		if (character == L' ') continue;
		if (character < 128) character = static_cast<unsigned char>(font->GetMappedChar(static_cast<SexyChar>(character)));
		bool found = false;
		for (auto& layer : font->mActiveLayerList)
		{
			auto glyph = layer.mBaseFontLayer->mCharDataMap.find(character);
			if (glyph != layer.mBaseFontLayer->mCharDataMap.end() && glyph->second.mWidth > 0) { found = true; break; }
		}
		if (!found) return false;
	}
	return true;
}

int CjkStoneButtonLabelWidth(const std::string& theLabel)
{
	if (LabelHasArtwork(theLabel)) return Sexy::FONT_DWARVENTODCRAFT18GREENINSET->StringWidth(theLabel);
	return ModText::TextWidth(ModText::GetFont(CjkPointSize(CJK_BUTTON_LABEL_PX), false), ModText::WideFromUtf8(theLabel.c_str()));
}

void CjkStoneButton::Draw(Graphics* g)
{
	if (mBtnNoDraw) return;

	if (LabelHasArtwork(mLabel))
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

	ModText::Font* aFont = ModText::GetFont(CjkPointSize(CJK_BUTTON_LABEL_PX), false);
	std::wstring aLabel = ModText::WideFromUtf8(mLabel.c_str());
	aFontX += (mWidth - ModText::TextWidth(aFont, aLabel)) / 2;
	aFontY += (mHeight - ModText::LineHeight(aFont)) / 2;
	// 亮绿 (0,196,0) = 位图按钮字的标准绿（1:1 截图对 OK/CANCEL 取色）；旧色
	// (47,107,43) 太暗、小字在石头上对比不够（2026-10-04 用户反馈）。位图字自带
	// 一圈暗烧边、宽字符直绘没有——四邻先垫 1px 暗底再压亮字。
	static const int anOutline[4][2] = { { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 } };
	for (int i = 0; i < 4; i++)
		ModText::DrawTextWide(g, aFont, aFontX + anOutline[i][0], aFontY + anOutline[i][1], aLabel,
			Color(16, 44, 16), g->mClipRect);
	ModText::DrawTextWide(g, aFont, aFontX, aFontY, aLabel,
		mIsOver ? Color(0x9B, 0xF0, 0x60) : Color(0x00, 0xC4, 0x00), g->mClipRect);
}
