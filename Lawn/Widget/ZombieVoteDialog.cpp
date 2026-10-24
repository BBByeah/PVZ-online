#include "ZombieVoteDialog.h"
#include "ZombieBuffButtonLabel.h"
#include "CjkStoneButton.h"
#include "RunChoiceLayout.h"
#include "../Run/RunState.h"
#include "../../LawnApp.h"
#include "../../ConstEnums.h"
#include "graphics/Graphics.h"
#include "../ModText.h"
#include <cstdio>

// @pvz-online: 僵尸强化投票框（MOD_BUILD 45，2026-10-14 用户令）。头文件里有分工说明，这里
// 只讲两件实现上的事：
// 1) 版式照 RunPickDialog（三列卡片 + 贴底一行）——列宽按候选说明整句量宽、封顶 220，
//    高度按"最多折几行 + 1 行已有"反推，以后改文案不用回来调数字。
// 2) 中文全部走宽字符直绘（ModText 顶对齐口径）——「弃权」「还剩」「已投」这些字不保证在
//    汉化像素位图字库里（1419 字，缺字画出来是空档不是豆腐块）；LawnDialog 自带的 header/
//    lines 两条位图字体路都留空。

ZombieVoteDialog::ZombieVoteDialog(LawnApp* theApp) : LawnDialog(
	theApp,
	Dialogs::DIALOG_ZOMBIE_VOTE,
	true,
	_S(""),
	_S(""),
	_S(""),
	Dialog::BUTTONS_NONE)
{
	mTitle = ModText::Tr("僵尸强化", "ZOMBIE BOOSTS");
	mHint = ModText::Tr("选择僵尸强化，票数最多的生效。", "Vote for a zombie boost; the most votes wins.");
	mAreaTop = 0;
	mAreaHeight = 0;
	mColumnWidth = 0;

	ModText::Font* aDescFont = ModText::GetFont(CjkPointSize(22), false);
	int aTargetColumn = 0;
	for (int i = 0; i < 3; i++)
	{
		mColumnX[i] = 0;
		// 候选 id 由 LawnApp 填好（房主抽、随 OPEN 广播；单机自抽）——这里只把名字挂上按钮。
		mChoiceButtons[i] = new CjkStoneButton(ZombieVoteDialog_Choice0 + i, this);
		mChoiceButtons[i]->SetLabel(theApp->mZombieVoteCandidates[i]<RunState::RUN_ZOMBIE_BUFF_COUNT?ZombieBuffButtonLabel(theApp->mZombieVoteCandidates[i]):ModText::Tr("无候选","Unavailable"));
        mChoiceButtons[i]->SetDisabled(theApp->mZombieVoteCandidates[i]>=RunState::RUN_ZOMBIE_BUFF_COUNT);
		mChoiceButtons[i]->mHasAlpha = true;
		mChoiceButtons[i]->mHasTransparencies = true;

		// 列宽按说明整句量宽（池子里没有显式 \n，不用按行拆）；长了交给折行——英文说明是
		// 长句，按整句推目标会一路把框顶到整屏宽（与三选一屏同一个 220 封顶）。
		int aWidth = ModText::TextWidth(aDescFont, ModText::WideFromUtf8(GetZombieBuffDesc(theApp->mZombieVoteCandidates[i])));
		if (aWidth > aTargetColumn) aTargetColumn = aWidth;
	}
	aTargetColumn += 12;
	if (aTargetColumn > 220) aTargetColumn = 220;

	// 「弃权」：对列出的三条都不表态的出路（47 改版：全队弃权=随机发 1 条，不裸奔）。
	mAbstainButton = new CjkStoneButton(ZombieVoteDialog_Abstain, this);
	mAbstainButton->SetLabel(ModText::Tr("弃权", "Abstain"));
	mAbstainButton->mHasAlpha = true;
	mAbstainButton->mHasTransparencies = true;

	mTallBottom = true;
	mVerticalCenterText = false;

	// 尺寸由内容反推（同 RunPickDialog 的账）：宽 = 三列目标；高 = 卡片区放下"最多折行数
	// + 1 行已有"。底部两行按钮（三张卡一行、弃权单独一行）比三选一屏多让一行——就是
	// Resize 里三张卡上移的"按钮高 + 6"，两处同一笔账。
	int aFitLineHeight = ModText::LineHeight(aDescFont) + 3;
	int aExtraHeight = IMAGE_BUTTON_LEFT->mHeight + 6;
	int aSizeX = 430;
	int aSizeY = 150 + aExtraHeight;
	CalcSize(aSizeX, aSizeY);
	// 宽、高各自达标即各自停长（两个条件绑一个 i 上的老病，三选一屏吃过一次）。
	for (int i = 1; i <= 12; i++)
	{
		int aMaxLines = 1;
		for (int j = 0; j < 3; j++)
		{
			int aLines = ModText::WrapLineCount(aDescFont, GetZombieBuffDesc(theApp->mZombieVoteCandidates[j]), mColumnWidth - 4);
			if (aLines > aMaxLines) aMaxLines = aLines;
		}
		int aNeededArea = (aMaxLines + 1) * aFitLineHeight + 6;
		bool aWidthDone = mColumnWidth >= aTargetColumn || mWidth >= 580;
		bool aHeightDone = mAreaHeight >= aNeededArea || mHeight >= 500 + aExtraHeight;
		if (aWidthDone && aHeightDone) break;
		if (!aWidthDone) aSizeX += 20;
		if (!aHeightDone) aSizeY += 20;
		CalcSize(aSizeX, aSizeY);
	}
	mApp->CenterDialog(this, mWidth, mHeight);
}

ZombieVoteDialog::~ZombieVoteDialog()
{
	for (int i = 0; i < 3; i++) delete mChoiceButtons[i];
	delete mAbstainButton;
}

void ZombieVoteDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	LawnDialog::Resize(theX, theY, theWidth, theHeight);

	int aButtonHeight = IMAGE_BUTTON_LEFT->mHeight;
	int aLeft = mContentInsets.mLeft + mBackgroundInsets.mLeft;
	int aContentWidth = mWidth - mContentInsets.mLeft - mContentInsets.mRight - mBackgroundInsets.mLeft - mBackgroundInsets.mRight;
	int aGap = 16;
	mColumnWidth = (aContentWidth - aGap * 2) / 3;
	for (int i = 0; i < 3; i++)
	{
		mColumnX[i] = aLeft + i * (mColumnWidth + aGap);
	}

	// 顶部三行（标题 / 说明 / 状态）——比三选一屏多一行状态，卡片区上沿跟着往下让。
	ModText::Font* aTitleFont = ModText::GetFont(CjkPointSize(28), true);
	ModText::Font* aLineFont = ModText::GetFont(CjkPointSize(22), false);
	int aTitleTop = mContentInsets.mTop + mBackgroundInsets.mTop + DIALOG_HEADER_OFFSET;
	mTitleY = aTitleTop;
	mHintY = aTitleTop + ModText::LineHeight(aTitleFont) + 8;
	mStatusY = mHintY + ModText::LineHeight(aLineFont) + 4;
	mAreaTop = mStatusY + ModText::LineHeight(aLineFont) + 10;

	// 底部两行：弃权贴底、三张卡在其上一行；卡片区的下沿从卡片行反推。
	int aAbstainY = mHeight - mContentInsets.mBottom - mBackgroundInsets.mBottom - aButtonHeight + 2;
	if (mTallBottom) aAbstainY += 5;
	int aButtonY = aAbstainY - aButtonHeight - 6;
	mAreaHeight = aButtonY - mAreaTop - 6;
	if (mAreaHeight < 0) mAreaHeight = 0;

	for (int i = 0; i < 3; i++)
	{
		mChoiceButtons[i]->Resize(mColumnX[i], aButtonY, mColumnWidth, aButtonHeight);
	}
	// 弃权按钮宽度按标签现量（标签是常量，量多少次都一样）——两字中文与 "Abstain" 各自合适。
	int aAbstainWidth = CjkStoneButtonWidth(CjkStoneButtonLabelWidth(mAbstainButton->mLabel) + 32, true);
	int aFootCenter = (mColumnX[0] + mColumnX[2] + mColumnWidth) / 2;
	mAbstainButton->Resize(aFootCenter - aAbstainWidth / 2, aAbstainY, aAbstainWidth, aButtonHeight);
}

void ZombieVoteDialog::AddedToManager(WidgetManager* theWidgetManager)
{
	LawnDialog::AddedToManager(theWidgetManager);
	for (int i = 0; i < 3; i++) AddWidget(mChoiceButtons[i]);
	AddWidget(mAbstainButton);
}

void ZombieVoteDialog::RemovedFromManager(WidgetManager* theWidgetManager)
{
	LawnDialog::RemovedFromManager(theWidgetManager);
	for (int i = 0; i < 3; i++) RemoveWidget(mChoiceButtons[i]);
	RemoveWidget(mAbstainButton);
}

void ZombieVoteDialog::Draw(Graphics* g)
{
	LawnDialog::Draw(g);

	// 标题 / 说明 / 状态三行逐行居中直绘。
	ModText::Font* aTitleFont = ModText::GetFont(CjkPointSize(28), true);
	ModText::Font* aLineFont = ModText::GetFont(CjkPointSize(22), false);
	std::wstring aTitle = ModText::WideFromUtf8(mTitle.c_str());
	ModText::DrawTextWide(g, aTitleFont, (mWidth - ModText::TextWidth(aTitleFont, aTitle)) / 2, mTitleY,
		aTitle, mColors[Dialog::COLOR_HEADER], g->mClipRect);
	std::wstring aHint = ModText::WideFromUtf8(mHint.c_str());
	ModText::DrawTextWide(g, aLineFont, (mWidth - ModText::TextWidth(aLineFont, aHint)) / 2, mHintY,
		aHint, mColors[Dialog::COLOR_LINES], g->mClipRect);

	// 状态行（倒计时 + 已投进度）：组串口径在 LawnApp::ZombieVoteStatusText——客户端只报
	// 自己的表态（票数在房主手里），倒计时两边同一个数（都从 OPEN 起算）。
	std::string aStatus = mApp->ZombieVoteStatusText();
	std::wstring aStatusW = ModText::WideFromUtf8(aStatus.c_str());
	ModText::DrawTextWide(g, aLineFont, (mWidth - ModText::TextWidth(aLineFont, aStatusW)) / 2, mStatusY,
		aStatusW, mColors[Dialog::COLOR_LINES], g->mClipRect);

	// 三列：效果说明贴在各自按钮上方（折行、逐列居中），列底一行「已有 n」——僵尸强化的
	// 层数是累积的，这一票投中的是"再叠一层"，得让人看得见现在有几层。
	for (int i = 0; i < 3; i++)
	{
		int aLineHeight = ModText::LineHeight(aLineFont) + 3;
		unsigned char aBuffId = mApp->mZombieVoteCandidates[i];
        if(aBuffId>=RunState::RUN_ZOMBIE_BUFF_COUNT)continue;
		Rect aRect(mColumnX[i] + 2, mAreaTop, mColumnWidth - 4, mAreaHeight - aLineHeight - 4);
		if (aRect.mHeight < 0) aRect.mHeight = 0;
		ModText::DrawWrappedCjk(g, RunChoiceLayout::DescriptionFont(GetZombieBuffDesc(aBuffId), aRect.mWidth, aRect.mHeight),
			aRect, GetZombieBuffDesc(aBuffId), mColors[Dialog::COLOR_LINES]);

		if (mApp->mRunState != nullptr)
		{
			char aCountUtf8[64];
			snprintf(aCountUtf8, sizeof(aCountUtf8), ModText::Tr("层数 %d", "Stacks %d"),
				mApp->mRunState->ZombieBuffCount(aBuffId));
			std::wstring aCountText = ModText::WideFromUtf8(aCountUtf8);
			int aCountY = mAreaTop + mAreaHeight - aLineHeight;
			if (aCountY < mAreaTop) aCountY = mAreaTop;
			int aCountX = mColumnX[i] + (mColumnWidth - ModText::TextWidth(aLineFont, aCountText)) / 2;
			ModText::DrawTextWide(g, aLineFont, aCountX, aCountY, aCountText,
				mColors[Dialog::COLOR_LINES], g->mClipRect);
		}
	}
}

void ZombieVoteDialog::ButtonDepress(int theId)
{
	int aChoice = -1;
	if (theId >= ZombieVoteDialog_Choice0 && theId <= ZombieVoteDialog_Choice2)
	{
		aChoice = theId - ZombieVoteDialog_Choice0;
	}
	else if (theId == ZombieVoteDialog_Abstain)
	{
		aChoice = RunState::RUN_ZOMBIE_VOTE_CHOICES;	// 3 = 弃权
	}
	if (aChoice < 0) return;

	// 本端一票锁定：点中的置按下态、四个按钮全置灰。客户端这张框要留到 RESULT 才关，
	// 这几秒里不能让人改主意——改主意在协议上就是第二票，房主按一席一票丢弃，改了也白改。
	for (int i = 0; i < 3; i++)
	{
		mChoiceButtons[i]->mDisabled = true;
		mChoiceButtons[i]->mInverted = (aChoice == i);
	}
	mAbstainButton->mDisabled = true;
	mAbstainButton->mInverted = (aChoice == RunState::RUN_ZOMBIE_VOTE_CHOICES);

	mApp->ZombieVoteChosen(aChoice);
}

// 键盘一律不认（同 RunPickDialog 纪律）：这一屏要么点一张卡、要么点「弃权」，键盘顺手带过
// 一个默认选择不是这张框的本意——它按秒计着时，两秒钟够把一行字读错。
void ZombieVoteDialog::KeyDown(KeyCode theKey)
{
	(void)theKey;
}
