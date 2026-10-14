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

// 八行的行数表（构造/摆位/注册/绘制共用；行序 = 槽位序 = 页序：
// 第 1 页「规模/节奏/植物僵尸」、第 2 页「滑动收阳光/巨型Boss/增益倍选」、
// 第 3 页「僵尸 buff/场景难度」）。
static const int kRowCounts[RunOptionsDialog::RUN_OPTIONS_ROW_COUNT] = { 4, 3, 2, 2, 2, 3, 3, 5 };

// 「场景难度」行的槽位号（行序末位）。无尽档不吃阶梯，该行整行不摆（2026-10-14 用户
// 裁决）——mShowLadderRow 为假时它永远不上场（Resize 去屏外，画不着、点不着）。
// 摆在末位还有个好处：无尽档末页只剩僵尸 buff 一行贴顶，不留空槽。
static const int kRowLadder = 7;

// 「场景难度」行的按钮顺序与枚举顺序不同：UI 按倍率从小到大摆（用户 2026-10-14 令
// 「做 1 1.2 1.33 1.5 2 五档」），枚举里 RUN_LADDER_STD（= 现状 ×1.33）是 0（sentinel
// 保老档不静默变难度）。value ↔ 槽位 的桥就这一张表：按钮 i 的值 = kLadderUiValues[i]。
static const int kLadderUiValues[5] = {
	RunState::RUN_LADDER_X1, RunState::RUN_LADDER_X12, RunState::RUN_LADDER_STD,
	RunState::RUN_LADDER_X15, RunState::RUN_LADDER_X2
};
static int LadderUiIndex(int theValue)
{
	for (int i = 0; i < 5; i++)
	{
		if (kLadderUiValues[i] == theValue) return i;
	}
	return 2;	// 非法档归「×1.33」槽位（= STD，与 LadderPermilleFor 的兜底同口径）
}

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

RunOptionsDialog::RunOptionsDialog(LawnApp* theApp, int theScaleSel, int theTempoSel, int theZombotanySel, int theSlideSel, int theBossSel, int theBuffMultSel, int theLadderSel, int theZombieBuffSel, bool theEndless)
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
	mZombieBuffSel = theZombieBuffSel;
	if (mZombieBuffSel < 0 || mZombieBuffSel > 2) mZombieBuffSel = 0;
	mLadderSel = theLadderSel;
	if (mLadderSel < RunState::RUN_LADDER_STD || mLadderSel > RunState::RUN_LADDER_X2) mLadderSel = RunState::RUN_LADDER_STD;
	mShowLadderRow = !theEndless;	// 无尽档不吃阶梯 → 整行不摆（2026-10-14 用户裁决）

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

	// 第七行「僵尸 buff」（2026-10-14 用户令，MOD_BUILD 45）：0/1/2 = 每进一个新场景给
	// 僵尸叠几条强化（全队投票选具体哪几条；无尽档五关一选）。0 = 整条机制不开。
	mZombieBuffCaption = ModText::Tr("僵尸 buff（每新场景）", "Zombie buffs (per new scene)");
	mZombieBuffLabels[0] = ModText::Tr("关闭", "Off");
	mZombieBuffLabels[1] = ModText::Tr("1 条", "1 buff");
	mZombieBuffLabels[2] = ModText::Tr("2 条", "2 buffs");

	// 第八行「场景难度」（2026-10-14 用户令，MOD_BUILD 45）：阶梯倍率乘在僵尸血量与
	// 出怪点数上；×1.33 = 现状（老档/默认；枚举值是 sentinel 0）。无尽档不吃阶梯，
	// 该行整行不摆（mShowLadderRow，构造参数 theEndless）。
	mLadderCaption = ModText::Tr("场景难度阶梯（血量+出怪）", "Scene difficulty ladder (HP+spawns)");
	mLadderLabels[0] = ModText::Tr("×1", "×1");
	mLadderLabels[1] = ModText::Tr("×1.2", "×1.2");
	mLadderLabels[2] = ModText::Tr("×1.33", "×1.33");
	mLadderLabels[3] = ModText::Tr("×1.5", "×1.5");
	mLadderLabels[4] = ModText::Tr("×2", "×2");

	mTitleY = 0;
	for (int i = 0; i < RUN_OPTIONS_ROW_COUNT; i++) mCaptionYs[i] = 0;

	// 各行按钮等宽（取最长标签量的）；石门贴图平铺对宽度有整段要求（见 CjkStoneButtonWidth）。
	// 量宽用按钮实际画标签的那档字号（CjkPointSize(CJK_BUTTON_LABEL_PX)），量画同一份。
	ModText::Font* aLabelFont = ModText::GetFont(CjkPointSize(CJK_BUTTON_LABEL_PX), false);
	const std::string* aLabelSets[RUN_OPTIONS_ROW_COUNT] = {
		mScaleLabels, mTempoLabels, mZombotanyLabels, mSlideLabels, mBossLabels, mBuffMultLabels,
		mZombieBuffLabels, mLadderLabels
	};
	ButtonWidget** aSlots[RUN_OPTIONS_ROW_COUNT] = {
		mScaleButtons, mTempoButtons, mZombotanyButtons, mSlideButtons, mBossButtons, mBuffMultButtons,
		mZombieBuffButtons, mLadderButtons
	};
	int* aWidthSets[RUN_OPTIONS_ROW_COUNT] = {
		mScaleWidths, mTempoWidths, mZombotanyWidths, mSlideWidths, mBossWidths, mBuffMultWidths,
		mZombieBuffWidths, mLadderWidths
	};
	static const int kRowIdBase[RUN_OPTIONS_ROW_COUNT] = {
		RunOptionsDialog_Scale0, RunOptionsDialog_Tempo0, RunOptionsDialog_Zombotany0,
		RunOptionsDialog_Slide0, RunOptionsDialog_Boss0, RunOptionsDialog_BuffMult0,
		RunOptionsDialog_ZombieBuff0, RunOptionsDialog_Ladder0
	};
	// 每行"选中槽位号"：多数行 = 值本身（枚举顺序 = 按钮顺序）；「场景难度」行例外
	// （UI 顺序与枚举顺序不同，走 kLadderUiValues 映射）。
	const int aSelIndex[RUN_OPTIONS_ROW_COUNT] = {
		mScaleSel, mTempoSel, mZombotanySel, mSlideSel, mBossSel, mBuffMultSel,
		mZombieBuffSel, LadderUiIndex(mLadderSel)
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
			aButton->mInverted = (i == aSelIndex[aRow]);
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

	// 版心（翻页批后按"一页"算）：宽 = 标题与八行按钮总宽的最大者；高 = 标题 + 间隔 + 三行
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
	// 2026-10-13 实机实测：CalcSize 的高度按对话框中段贴图块向上取整——算式上的微调会被
	// 取整吃掉（实测总高 614，超 800×600 屏高上下各 7px）。这里按「合法高度 = 上贴图 +
	// 下贴图 + 隔头 + k×中贴图块高」从取整结果往下逐块收，收到屏高以内（留 8px 边）。
	{
		int aBottomHeight = (mTallBottom ? IMAGE_DIALOG_BIGBOTTOMLEFT : IMAGE_DIALOG_BOTTOMLEFT)->mHeight;
		int aImageHeight = IMAGE_DIALOG_TOPLEFT->mHeight + aBottomHeight + DIALOG_HEADER_OFFSET;
		int aCenterHeight = IMAGE_DIALOG_CENTERLEFT->mHeight;
		while (mHeight > mApp->mHeight - 8 && mHeight - aCenterHeight >= aImageHeight)
			mHeight -= aCenterHeight;
	}
	mApp->CenterDialog(this, mWidth, mHeight);
	mClip = false;
}

RunOptionsDialog::~RunOptionsDialog()
{
	ButtonWidget** aSlots[RUN_OPTIONS_ROW_COUNT] = {
		mScaleButtons, mTempoButtons, mZombotanyButtons, mSlideButtons, mBossButtons, mBuffMultButtons,
		mZombieBuffButtons, mLadderButtons
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
// 无尽档的「场景难度」行永远不上场（mShowLadderRow = 假，同"挪去屏外"处理）。
void RunOptionsDialog::PlaceRows()
{
	ModText::Font* aTitleFont = ModText::GetFont(16, true);
	ModText::Font* aCaptionFont = ModText::GetFont(12, false);
	int aButtonHeight = IMAGE_BUTTON_LEFT->mHeight;

	ButtonWidget** aSlots[RUN_OPTIONS_ROW_COUNT] = {
		mScaleButtons, mTempoButtons, mZombotanyButtons, mSlideButtons, mBossButtons, mBuffMultButtons,
		mZombieBuffButtons, mLadderButtons
	};
	int* aWidthSets[RUN_OPTIONS_ROW_COUNT] = {
		mScaleWidths, mTempoWidths, mZombotanyWidths, mSlideWidths, mBossWidths, mBuffMultWidths,
		mZombieBuffWidths, mLadderWidths
	};

	int aTitleTop = mContentInsets.mTop + mBackgroundInsets.mTop + DIALOG_HEADER_OFFSET;
	mTitleY = aTitleTop;						// ModText 顶对齐：存的直接是顶

	int aRowTop = aTitleTop + ModText::LineHeight(aTitleFont) + 18;
	int aRowHeight = ModText::LineHeight(aCaptionFont) + 6 + aButtonHeight + 8;
	for (int aRow = 0; aRow < RUN_OPTIONS_ROW_COUNT; aRow++)
	{
		int aRowY = aRowTop + (aRow % RUN_OPTIONS_ROWS_PER_PAGE) * aRowHeight;
		bool aOnPage = (aRow / RUN_OPTIONS_ROWS_PER_PAGE == mPage)
			&& (aRow != kRowLadder || mShowLadderRow);
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
		mScaleButtons, mTempoButtons, mZombotanyButtons, mSlideButtons, mBossButtons, mBuffMultButtons,
		mZombieBuffButtons, mLadderButtons
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
		mScaleButtons, mTempoButtons, mZombotanyButtons, mSlideButtons, mBossButtons, mBuffMultButtons,
		mZombieBuffButtons, mLadderButtons
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
		&mScaleCaption, &mTempoCaption, &mZombotanyCaption, &mSlideCaption, &mBossCaption, &mBuffMultCaption,
		&mZombieBuffCaption, &mLadderCaption
	};
	for (int aRow = 0; aRow < RUN_OPTIONS_ROW_COUNT; aRow++)
	{
		if (aRow / RUN_OPTIONS_ROWS_PER_PAGE != mPage) continue;
		if (aRow == kRowLadder && !mShowLadderRow) continue;	// 无尽档不摆（同 PlaceRows）
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
	if (theId >= RunOptionsDialog_ZombieBuff0 && theId <= RunOptionsDialog_ZombieBuff0 + 2)
	{
		mZombieBuffSel = theId - RunOptionsDialog_ZombieBuff0;
		for (int i = 0; i < 3; i++) mZombieBuffButtons[i]->mInverted = (i == mZombieBuffSel);
		return;
	}
	// 「场景难度」：按钮槽位 ≠ 枚举值（UI 按倍率从小到大，枚举 STD=0 是第三枚），
	// 经 kLadderUiValues 换算；按下态也按"值相等"刷新。
	if (theId >= RunOptionsDialog_Ladder0 && theId <= RunOptionsDialog_Ladder0 + 4)
	{
		mLadderSel = kLadderUiValues[theId - RunOptionsDialog_Ladder0];
		for (int i = 0; i < 5; i++) mLadderButtons[i]->mInverted = (kLadderUiValues[i] == mLadderSel);
		return;
	}
	// 翻页（2026-10-13）：三页环绕翻——翻完重摆一次（当前页行上场、其余挪去屏外）。
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
