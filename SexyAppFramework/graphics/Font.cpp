#include "Font.h"
#include "Image.h"

using namespace Sexy;

_Font::_Font()
{	
	mAscent = 0;
	mAscentPadding = 0;
	mHeight = 0;
	mLineSpacingOffset = 0;
}

_Font::_Font(const _Font& theFont) :
	mAscent(theFont.mAscent),
	mAscentPadding(theFont.mAscentPadding),
	mHeight(theFont.mHeight),
	mLineSpacingOffset(theFont.mLineSpacingOffset)
{
}

_Font::~_Font()
{
}

int	_Font::GetAscent()
{
	return mAscent;
}

int	_Font::GetAscentPadding()
{
	return mAscentPadding;
}

int	_Font::GetDescent()
{
	return mHeight - mAscent;
}

int	_Font::GetHeight()
{
	return mHeight;
}

int _Font::GetLineSpacingOffset()
{
	return mLineSpacingOffset;
}

int _Font::GetLineSpacing()
{
	return mHeight + mLineSpacingOffset;
}


int _Font::StringWidth(const SexyString&)
{
	return 0;
}


int _Font::CharWidth(wchar_t theChar)
{
	// @pvz-online: 汉化批 ②A——基类兜底：把码位编码成 UTF-8 再量宽（ASCII 与旧行为一致）
	SexyString aString;
	if ((unsigned)theChar < 0x80)
		aString.append(1, (SexyChar)theChar);
	else if (theChar < 0x800)
	{
		aString.append(1, (SexyChar)(0xC0 | (theChar >> 6)));
		aString.append(1, (SexyChar)(0x80 | (theChar & 0x3F)));
	}
	else
	{
		aString.append(1, (SexyChar)(0xE0 | (theChar >> 12)));
		aString.append(1, (SexyChar)(0x80 | ((theChar >> 6) & 0x3F)));
		aString.append(1, (SexyChar)(0x80 | (theChar & 0x3F)));
	}
	return StringWidth(aString);
}

int _Font::CharWidthKern(wchar_t theChar, wchar_t)
{
	return CharWidth(theChar);
}

void _Font::DrawString(Graphics*, int, int, const SexyString&, const Color&, const Rect&){}

