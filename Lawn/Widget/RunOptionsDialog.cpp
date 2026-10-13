#include "RunOptionsDialog.h"
#include "CjkStoneButton.h"
#include "GameButton.h"
#include "../../LawnApp.h"
#include "../../Resources.h"
#include "../../ConstEnums.h"
#include "../Run/RunState.h"
#include "../../Sexy.TodLib/TodCommon.h"
#include "graphics/Font.h"
#include "graphics/Graphics.h"
#include "widget/WidgetManager.h"
#include "../ModText.h"
#include <string>

// 行内按钮间距 / 底部两枚的间距（RunModeDialog 的难度行与底行各用一套，这里照抄）。
static const int kGapX = 12;
static const int kBottomGapX = 20;

// 六行的行数表（构造/摆位/注册/绘制共用；行序 = 槽位序 = 页序：
// 第 1 页「规模/节奏/植物僵尸」、第 2 页「滑动收阳光/巨型Boss/增益倍选」）。
static const int kRowCounts[RunOptionsDialog::RUN_OPTIONS_ROW_COUNT] = { 4, 3, 2, 2, 2, 3 };

// 小标题居中直绘（RunModeDialog 的难度行同款颜色）。
static void DrawRowCaption(Graphics* g, ModText::Font* theFont, const std::string& theText, int theY, int theWidth)
{
	std::wstring aWide = ModText::WideFromUtf8(theText.c_str());
	ModText::DrawTextWide(g, theFont, (theWidth - ModText::TextWidth(theFont, aWide)) / 2, theY,
		aWide, Color(96, 72, 40), g->mClipRect);
}

// 翻页行中间那句「第 n / m 页」（中英各一版；两处调用：构造量宽、Draw 现画）。
static std::string MakePageText(int thePage)
{
	return ModText::Tr("第 ", "Page ") + std::to_string(thePage + 1) + " / "
		+ std::to_string(RunOptionsDialog::RUN_OPTIONS_PAGE_COUNT) + ModText::Tr(" 页", "");
}

RunOptionsDialog::RunOptionsDialog(LawnApp* theApp, int theScaleSel, int theTempoSel, int theZombotanySel, int theSlideSel, int theBossSel, int theBuffMultSel)
	: LawnDialog(theApp, Dialogs::DIALOG_RUN_OPTIONS, true, _S(""), _S(""), _S(""), Dialog::BUTTONS_NONE)
{
	mTitle = ModText::Tr("高级选项", "Advanced Options");

	// 传入的当前值先钳一遍（与 AlignRunToHost 同纪律）：非法档按 标准/标准/关。
	mScaleSel = theScaleSel;
	mTempoSel = theTempoSel;
	mZombotanySel = theZombotanySel;
	if (mScaleSel < RunState::RUN_SCALE_HALF || mScaleSel > RunState::RUN_SCALE_QUAD) mScaleSel = RunState::RUN_SCALE_STD;
	if (mTempoSel < RunState::RUN_TEMPO_FAST || mTempoSel > RunState::RUN_TEMPO_SLOW) mTempoSel = RunState::RUN_TEMPO_STD;
	if (mZombotanySel != 0 && mZombotanySel != 1) mZombotanySel = 0;
	mSlideSel = theSlideSel;
	if (mSlideSel != 0 && mSlideSel != 1) mSlideSel = 1;	// 非法档归开（同"默认开"口径）
	mBossSel = theBossSel;
	if (mBossSel != 0 && mBossSel != 1) mBossSel = 0;
	mBuffMultSel = theBuffMultSel;
	if (mBuffMultSel < RunState::RUN_BUFFMULT_X1 || mBuffMultSel > RunState::RUN_BUFFMULT_X3) mBuffMultSel = RunState::RUN_BUFFMULT_X1;

	mScaleCaption = ModText::Tr("出怪规模（全队倍率）", "Zombie scale (team multiplier)");
	mScaleLabels[0] = ModText::Tr("少 ×0.5", "Few ×0.5");
	mScaleLabels[1] = ModText::Tr("标准 ×1", "Standard ×1");
	mScaleLabels[2] = ModText::Tr("多 ×2", "Many ×2");
	mScaleLabels[3] = ModText::Tr("海量 ×4", "Swarm ×4");

	mTempoCaption = ModText::Tr("出怪节奏（波间隔）", "Spawn tempo (wave interval)");
	mTempoLabels[0] = ModText::Tr("快 ×0.6", "Fast ×0.6");
	mTempoLabels[1] = ModText::Tr("标准 ×1", "Standard ×1");
	mTempoLabels[2] = ModText::Tr("慢 ×1.6", "Slow ×1.6");

	mZombotanyCaption = ModText::Tr("植物僵尸（实验玩法）", "ZomBotany (experimental)");
	mZombotanyLabels[0] = ModText::Tr("关闭", "Off");
	mZombotanyLabels[1] = ModText::Tr("开启", "On");

	mSlideCaption = ModText::Tr("滑动收阳光（仅本机，全局生效）", "Slide-collect sun (local, all modes)");
	mSlideLabels[0] = ModText::Tr("关闭", "Off");
	mSlideLabels[1] = ModText::Tr("开启", "On");

	mBossCaption = ModText::Tr("巨型 Boss（闯关关底）", "Giant Boss (run finale)");
	mBossLabels[0] = ModText::Tr("关闭", "Off");
	mBossLabels[1] = ModText::Tr("开启", "On");

	// 第六行「增益倍选」（2026-10-13 用户令，MOD_BUILD 43）：×1/×2/×3——每屏照旧三选一
	// 拿 1 条，但发放的三选一屏数乘倍（拿到的总数翻倍，代价是连点更多屏）。
	mBuffMultCaption = ModText::Tr("增益倍选（屏数倍率）", "Buff picks (multiplier)");
	mBuffMultLabels[0] = ModText::Tr("×1", "×1");
	mBuffMultLabels[1] = ModText::Tr("×2", "×2");
	mBuffMultLabels[2] = ModText::Tr("×3", "×3");

	mTitleY = 0;
	for (int i = 0; i < RUN_OPTIONS_ROW_COUNT; i++) mCaptionYs[i] = 0;

	// 各行按钮等宽（取最长标签量的）；石门贴图平铺对宽度有整段要求（见 CjkStoneButtonWidth）。
	// 量宽用按钮实际画标签的那档字号（CjkPointSize(CJK_BUTTON_LABEL_PX)），量画同一份。
	ModText::Font* aLabelFont = ModText::GetFont(CjkPointSize(CJK_BUTTON_LABEL_PX), false);
	const std::string* aLabelSets[RUN_OPTIONS_ROW_COUNT] = {
		mScaleLabels, mTempoLabels, mZombotanyLabels, mSlideLabels, mBossLabels, mBuffMultLabels
	};
	ButtonWidget** aSlots[RUN_OPTIONS_ROW_COUNT] = {
		mScaleButtons, mTempoButtons, mZombotanyButtons, mSlideButtons, mBossButtons, mBuffMultButtons
	};
	int* aWidthSets[RUN_OPTIONS_ROW_COUNT] = {
		mScaleWidths, mTempoWidths, mZombotanyWidths, mSlideWidths, mBossWidths, mBuffMultWidths
	};
	static const int kRowIdBase[RUN_OPTIONS_ROW_COUNT] = {
		RunOptionsDialog_Scale0, RunOptionsDialog_Tempo0, RunOptionsDialog_Zombotany0,
		RunOptionsDialog_Slide0, RunOptionsDialog_Boss0, RunOptionsDialog_BuffMult0
	};
	const int* aSelValues[RUN_OPTIONS_ROW_COUNT] = {
		&mScaleSel, &mTempoSel, &mZombotanySel, &mSlideSel, &mBossSel, &mBuffMultSel
	};
	for (int aRow = 0; aRow < RUN_OPTIONS_ROW_COUNT; aRow++)
	{
		int aLabelMax = 0;
		for (int i = 0; i < kRowCounts[aRow]; i++)
		{
			int aWidth = ModText::TextWidth(aLabelFont, ModText::WideFromUtf8(aLabelSets[aRow][i].c_str()));
			if (aWidth > aLabelMax) aLabelMax = aWidth;
		}
		int aButtonWidth = CjkStoneButtonWidth(aLabelMax + 26, true);
		for (int i = 0; i < kRowCounts[aRow]; i++)
		{
			aWidthSets[aRow][i] = aButtonWidth;

			CjkStoneButton* aButton = new CjkStoneButton(kRowIdBase[aRow] + i, this);
			aButton->SetLabel(aLabelSets[aRow][i]);
			aButton->mHasAlpha = true;
			aButton->mHasTransparencies = true;
			aButton->mInverted = (i == *aSelValues[aRow]);
			aSlots[aRow][i] = aButton;
		}
	}

	mOkButton = new CjkStoneButton(RunOptionsDialog_OK, this);
	mOkButton->SetLabel(ModText::Tr("确定", "OK"));
	mOkButton->mHasAlpha = true;
	mOkButton->mHasTransparencies = true;
	mOkWidth = CjkStoneButtonWidth(ModText::TextWidth(ModText::GetFont(CjkPointSize(CJK_BUTTON_LABEL_PX), false),
		ModText::WideFromUtf8(mOkButton->mLabel.c_str())) + 32, true);

	mCancelButton = new CjkStoneButton(Dialog::ID_NO, this);
	mCancelButton->SetLabel(ModText::Tr("取消", "Cancel"));
	mCancelButton->mHasAlpha = true;
	mCancelButton->mHasTransparencies = true;
	mCancelWidth = CjkStoneButtonWidth(ModText::TextWidth(ModText::GetFont(CjkPointSize(CJK_BUTTON_LABEL_PX), false),
		ModText::WideFromUtf8(mCancelButton->mLabel.c_str())) + 32, true);

	// 翻页行两枚（上一页 / 下一页，环绕翻页）+ 中间的「第 n / m 页」文字（宽度先量好，摆位留位）。
	mPrevButton = new CjkStoneButton(RunOptionsDialog_PrevPage, this);
	mPrevButton->SetLabel(ModText::Tr("上一页", "Prev"));
	mPrevButton->mHasAlpha = true;
	mPrevButton->mHasTransparencies = true;
	mPrevWidth = CjkStoneButtonWidth(ModText::TextWidth(ModText::GetFont(CjkPointSize(CJK_BUTTON_LABEL_PX), false),
		ModText::WideFromUtf8(mPrevButton->mLabel.c_str())) + 26, true);

	mNextButton = new CjkStoneButton(RunOptionsDialog_NextPage, this);
	mNextButton->SetLabel(ModText::Tr("下一页", "Next"));
	mNextButton->mHasAlpha = true;
	mNextButton->mHasTransparencies = true;
	mNextWidth = CjkStoneButtonWidth(ModText::TextWidth(ModText::GetFont(CjkPointSize(CJK_BUTTON_LABEL_PX), false),
		ModText::WideFromUtf8(mNextButton->mLabel.c_str())) + 26, true);

	mPageTextW = ModText::TextWidth(ModText::GetFont(12, false), ModText::WideFromUtf8(MakePageText(0).c_str()));
	mPageTextX = mPageTextY = 0;

	mPage = 0;

	mTallBottom = true;
	mVerticalCenterText = false;

	// 版心（翻页批后按"一页"算）：宽 = 标题与六行按钮总宽的最大者；高 = 标题 + 间隔 + 三行
	// （小标题 + 按钮）+ 翻页行 + 确定行。CalcSize 会按对话框贴图再取整/加高，多出来的
	// 空隙由 PlaceRows 里"首行贴顶、翻页/确定贴底"吸收。
	ModText::Font* aTitleFont = ModText::GetFont(16, true);
	ModText::Font* aCaptionFont = ModText::GetFont(12, false);
	int aButtonHeight = IMAGE_BUTTON_LEFT->mHeight;
	int anExtraX = ModText::TextWidth(aTitleFont, ModText::WideFromUtf8(mTitle.c_str())) + 80;
	for (int aRow = 0; aRow < RUN_OPTIONS_ROW_COUNT; aRow++)
	{
		int aRowWidth = 0;
		for (int i = 0; i < kRowCounts[aRow]; i++)
		{
			if (i > 0) aRowWidth += kGapX;
			aRowWidth += aWidthSets[aRow][i];
		}
		if (aRowWidth > anExtraX) anExtraX = aRowWidth;
	}
	int aRowHeight = ModText::LineHeight(aCaptionFont) + 6 + aButtonHeight + 8;
	int anExtraY = ModText::LineHeight(aTitleFont) + 18		// 标题 + 与首行的间隔
		+ aRowHeight * RUN_OPTIONS_ROWS_PER_PAGE			// 一页三行
		+ aButtonHeight + 12								// 翻页行 + 间隔
		+ aButtonHeight + 18;								// 确定行 + 与上一行的间隔

	CalcSize(anExtraX, anExtraY);
	mApp->CenterDialog(this, mWidth, mHeight);
	mClip = false;
}

RunOptionsDialog::~RunOptionsDialog()
{
	ButtonWidget** aSlots[RUN_OPTIONS_ROW_COUNT] = {
		mScaleButtons, mTempoButtons, mZombotanyButtons, mSlideButtons, mBossButtons, mBuffMultButtons
	};
	for (int aRow = 0; aRow < RUN_OPTIONS_ROW_COUNT; aRow++)
	{
		for (int i = 0; i < kRowCounts[aRow]; i++) delete aSlots[aRow][i];
	}
	delete mOkButton;
	delete mCancelButton;
	delete mPrevButton;
	delete mNextButton;
}

// 一次摆好全部控件（Resize 与翻页共用）。一页三行：行槽位固定（页只决定谁上场）——
// 当前页的行摆进槽位，其余整行挪去负坐标（画不着、点不着；选中态留在各按钮的 mInverted 上，
// 翻回来原样在）。翻页行与确定行都贴底算，中部的多余空隙吸收在末行与翻页行之间。
void RunOptionsDialog::PlaceRows()
{
	ModText::Font* aTitleFont = ModText::GetFont(16, true);
	ModText::Font* aCaptionFont = ModText::GetFont(12, false);
	int aButtonHeight = IMAGE_BUTTON_LEFT->mHeight;

	ButtonWidget** aSlots[RUN_OPTIONS_ROW_COUNT] = {
		mScaleButtons, mTempoButtons, mZombotanyButtons, mSlideButtons, mBossButtons, mBuffMultButtons
	};
	int* aWidthSets[RUN_OPTIONS_ROW_COUNT] = {
		mScaleWidths, mTempoWidths, mZombotanyWidths, mSlideWidths, mBossWidths, mBuffMultWidths
	};

	int aTitleTop = mContentInsets.mTop + mBackgroundInsets.mTop + DIALOG_HEADER_OFFSET;
	mTitleY = aTitleTop;						// ModText 顶对齐：存的直接是顶

	int aRowTop = aTitleTop + ModText::LineHeight(aTitleFont) + 18;
	int aRowHeight = ModText::LineHeight(aCaptionFont) + 6 + aButtonHeight + 8;
	for (int aRow = 0; aRow < RUN_OPTIONS_ROW_COUNT; aRow++)
	{
		int aRowY = aRowTop + (aRow % RUN_OPTIONS_ROWS_PER_PAGE) * aRowHeight;
		bool aOnPage = (aRow / RUN_OPTIONS_ROWS_PER_PAGE == mPage);
		mCaptionYs[aRow] = aOnPage ? aRowY : -2000;
		int aButtonY = aOnPage ? aRowY + ModText::LineHeight(aCaptionFont) + 6 : -2000;

		int aRowWidth = 0;
		for (int i = 0; i < kRowCounts[aRow]; i++)
		{
			if (i > 0) aRowWidth += kGapX;
			aRowWidth += aWidthSets[aRow][i];
		}
		int aX = (mWidth - aRowWidth) / 2;
		for (int i = 0; i < kRowCounts[aRow]; i++)
		{
			aSlots[aRow][i]->Resize(aX, aButtonY, aWidthSets[aRow][i], aButtonHeight);
			aX += aWidthSets[aRow][i] + kGapX;
		}
	}

	// 确定行贴底，两枚并排居中（原算式）；翻页行贴在确定行上面一行。
	int aBottomY = mHeight - mContentInsets.mBottom - mBackgroundInsets.mBottom - aButtonHeight + 2;
	if (mTallBottom) aBottomY += 5;
	int aTotalW = mOkWidth + mCancelWidth + kBottomGapX;
	int aLeft = (mWidth - aTotalW) / 2;
	mOkButton->Resize(aLeft, aBottomY, mOkWidth, aButtonHeight);
	mCancelButton->Resize(aLeft + mOkWidth + kBottomGapX, aBottomY, mCancelWidth, aButtonHeight);

	// 翻页行：上一页 [第 n / m 页] 下一页，三件套横排居中（页码文字是 Draw 画的，这里只留位）。
	int aNavY = aBottomY - 12 - aButtonHeight;
	int aNavTotalW = mPrevWidth + kGapX + mPageTextW + kGapX + mNextWidth;
	int aNavLeft = (mWidth - aNavTotalW) / 2;
	mPrevButton->Resize(aNavLeft, aNavY, mPrevWidth, aButtonHeight);
	mPageTextX = aNavLeft + mPrevWidth + kGapX;
	mPageTextY = aNavY + (aButtonHeight - ModText::LineHeight(aCaptionFont)) / 2;
	mNextButton->Resize(mPageTextX + mPageTextW + kGapX, aNavY, mNextWidth, aButtonHeight);
}

void RunOptionsDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	LawnDialog::Resize(theX, theY, theWidth, theHeight);
	PlaceRows();
}

void RunOptionsDialog::AddedToManager(WidgetManager* theWidgetManager)
{
	LawnDialog::AddedToManager(theWidgetManager);
	ButtonWidget** aSlots[RUN_OPTIONS_ROW_COUNT] = {
		mScaleButtons, mTempoButtons, mZombotanyButtons, mSlideButtons, mBossButtons, mBuffMultButtons
	};
	for (int aRow = 0; aRow < RUN_OPTIONS_ROW_COUNT; aRow++)
	{
		for (int i = 0; i < kRowCounts[aRow]; i++) AddWidget(aSlots[aRow][i]);
	}
	AddWidget(mOkButton);
	AddWidget(mCancelButton);
	AddWidget(mPrevButton);
	AddWidget(mNextButton);
}

void RunOptionsDialog::RemovedFromManager(WidgetManager* theWidgetManager)
{
	LawnDialog::RemovedFromManager(theWidgetManager);
	ButtonWidget** aSlots[RUN_OPTIONS_ROW_COUNT] = {
		mScaleButtons, mTempoButtons, mZombotanyButtons, mSlideButtons, mBossButtons, mBuffMultButtons
	};
	for (int aRow = 0; aRow < RUN_OPTIONS_ROW_COUNT; aRow++)
	{
		for (int i = 0; i < kRowCounts[aRow]; i++) RemoveWidget(aSlots[aRow][i]);
	}
	RemoveWidget(mOkButton);
	RemoveWidget(mCancelButton);
	RemoveWidget(mPrevButton);
	RemoveWidget(mNextButton);
}

void RunOptionsDialog::Draw(Graphics* g)
{
	LawnDialog::Draw(g);   // 底板。header / lines 都是空的，只出框。

	if (!mTitle.empty())
	{
		ModText::Font* aTitleFont = ModText::GetFont(16, true);
		std::wstring aTitle = ModText::WideFromUtf8(mTitle.c_str());
		ModText::DrawTextWide(g, aTitleFont,
			(mWidth - ModText::TextWidth(aTitleFont, aTitle)) / 2, mTitleY,
			aTitle, mColors[Dialog::COLOR_HEADER], g->mClipRect);
	}

	// 当前页三行的小标题（各行的按钮自己画自己，走控件那套；非当前页的行已被挪去屏外）。
	ModText::Font* aCaptionFont = ModText::GetFont(12, false);
	const std::string* aCaptions[RUN_OPTIONS_ROW_COUNT] = {
		&mScaleCaption, &mTempoCaption, &mZombotanyCaption, &mSlideCaption, &mBossCaption, &mBuffMultCaption
	};
	for (int aRow = 0; aRow < RUN_OPTIONS_ROW_COUNT; aRow++)
	{
		if (aRow / RUN_OPTIONS_ROWS_PER_PAGE != mPage) continue;
		DrawRowCaption(g, aCaptionFont, *aCaptions[aRow], mCaptionYs[aRow], mWidth);
	}

	// 翻页行中间的「第 n / m 页」。
	std::wstring aPage = ModText::WideFromUtf8(MakePageText(mPage).c_str());
	ModText::DrawTextWide(g, aCaptionFont, mPageTextX, mPageTextY,
		aPage, Color(96, 72, 40), g->mClipRect);
}

// 键盘一律不认（同 OnlineStartDialog / RunModeDialog 的纪律）。
void RunOptionsDialog::KeyDown(KeyCode theKey)
{
	(void)theKey;
}

// 0x4572E0 的同一支音效（LawnDialog::ButtonPress），听着还是原版的石头按钮
void RunOptionsDialog::ButtonPress(int theId)
{
	(void)theId;
	mApp->PlaySample(Sexy::SOUND_GRAVEBUTTON);
}

void RunOptionsDialog::ButtonDepress(int theId)
{
	// 不调 Dialog::ButtonDepress（RunModeDialog 同款纪律）：结果由打开方在 WaitForResult
	// 里收，不走 2000+/3000+ 的标准对话框路由。
	if (theId >= RunOptionsDialog_Scale0 && theId <= RunOptionsDialog_Scale0 + 3)
	{
		mScaleSel = theId - RunOptionsDialog_Scale0;
		for (int i = 0; i < 4; i++) mScaleButtons[i]->mInverted = (i == mScaleSel);
		return;
	}
	if (theId >= RunOptionsDialog_Tempo0 && theId <= RunOptionsDialog_Tempo0 + 2)
	{
		mTempoSel = theId - RunOptionsDialog_Tempo0;
		for (int i = 0; i < 3; i++) mTempoButtons[i]->mInverted = (i == mTempoSel);
		return;
	}
	if (theId >= RunOptionsDialog_Zombotany0 && theId <= RunOptionsDialog_Zombotany0 + 1)
	{
		mZombotanySel = theId - RunOptionsDialog_Zombotany0;
		for (int i = 0; i < 2; i++) mZombotanyButtons[i]->mInverted = (i == mZombotanySel);
		return;
	}
	if (theId >= RunOptionsDialog_Slide0 && theId <= RunOptionsDialog_Slide0 + 1)
	{
		mSlideSel = theId - RunOptionsDialog_Slide0;
		for (int i = 0; i < 2; i++) mSlideButtons[i]->mInverted = (i == mSlideSel);
		return;
	}
	if (theId >= RunOptionsDialog_Boss0 && theId <= RunOptionsDialog_Boss0 + 1)
	{
		mBossSel = theId - RunOptionsDialog_Boss0;
		for (int i = 0; i < 2; i++) mBossButtons[i]->mInverted = (i == mBossSel);
		return;
	}
	if (theId >= RunOptionsDialog_BuffMult0 && theId <= RunOptionsDialog_BuffMult0 + 2)
	{
		mBuffMultSel = theId - RunOptionsDialog_BuffMult0;
		for (int i = 0; i < 3; i++) mBuffMultButtons[i]->mInverted = (i == mBuffMultSel);
		return;
	}
	// 翻页（2026-10-13）：两页环绕翻——翻完重摆一次（当前页行上场、其余挪去屏外）。
	if (theId == RunOptionsDialog_PrevPage || theId == RunOptionsDialog_NextPage)
	{
		if (theId == RunOptionsDialog_PrevPage)
			mPage = (mPage + RUN_OPTIONS_PAGE_COUNT - 1) % RUN_OPTIONS_PAGE_COUNT;
		else
			mPage = (mPage + 1) % RUN_OPTIONS_PAGE_COUNT;
		PlaceRows();
		return;
	}
	if (theId == RunOptionsDialog_OK)
	{
		mResult = RunOptionsDialog_OK;	// 确定 = 收结果（WaitForResult 自己收摊）
		return;
	}
	if (theId == Dialog::ID_NO)
	{
		mResult = Dialog::ID_NO;		// 取消 = 关弹窗不改任何值
		return;
	}
}
