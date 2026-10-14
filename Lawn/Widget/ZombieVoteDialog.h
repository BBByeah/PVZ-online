#ifndef __ZOMBIEVOTEDIALOG_H__
#define __ZOMBIEVOTEDIALOG_H__

#include "LawnDialog.h"

class CjkStoneButton;

// @pvz-online: 僵尸强化投票框（MOD_BUILD 45，2026-10-14 用户令）。每进一个新场景（无尽
// 五关一选）给僵尸叠 0/1/2 条强化，全队投票定条目：三张候选卡 + 一枚「弃权」+ 30 秒
// 倒计时（超时 = 弃权，等待必须有出路）。单人局同一张框（只是跳过协议）。
//
// 归属单一：候选、计票、超时、结算全在 LawnApp（ZombieVoteChosen / UpdateZombieVote /
// ZombieVoteResolve），这个类只管画与收点击——按下第 N 张就把序号交回
// LawnApp::ZombieVoteChosen（3 = 弃权）。
//
// 文案纪律：中文全部走宽字符直绘（标题/说明/状态行 + CjkStoneButton 标签），不碰位图
// 字库——「弃权」这类字不保证在 1419 字汉化像素字库里（缺字画出来是空档不是豆腐块）。
class ZombieVoteDialog : public LawnDialog
{
public:
	enum
	{
		ZombieVoteDialog_Choice0 = 220,	// 三张候选卡（控件 id 偏移 = 候选下标）
		ZombieVoteDialog_Choice1,
		ZombieVoteDialog_Choice2,
		ZombieVoteDialog_Abstain		// 「弃权」：本场景明确不加（不参与兜底随机）
	};

	CjkStoneButton*	mChoiceButtons[3];
	CjkStoneButton*	mAbstainButton;
	int				mColumnX[3];		// 三列的位置与宽度（Resize 里算好，Draw 直接用）
	int				mColumnWidth;
	int				mAreaTop;			// 卡片区（说明 + 「已有 n」）的上沿与高度
	int				mAreaHeight;
	int				mTitleY;			// 标题顶部（ModText 顶对齐口径）
	int				mHintY;				// 说明行（每进一个新场景…）顶部
	int				mStatusY;			// 状态行（倒计时/已投）顶部
	std::string		mTitle;				// UTF-8 原样（绘制走 ModText::WideFromUtf8）
	std::string		mHint;				// UTF-8 原样（Tr 择串后的中文或英文）

public:
	ZombieVoteDialog(LawnApp* theApp);
	virtual ~ZombieVoteDialog();

	virtual void		Draw(Graphics* g);
	virtual void		Resize(int theX, int theY, int theWidth, int theHeight);
	virtual void		AddedToManager(WidgetManager* theWidgetManager);
	virtual void		RemovedFromManager(WidgetManager* theWidgetManager);
	virtual void		ButtonDepress(int theId);
	virtual void		KeyDown(KeyCode theKey);
};

#endif
