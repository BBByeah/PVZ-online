#include "OnlineSeatDialog.h"
#include "OnlineStatusWidget.h"
#include "CjkStoneButton.h"
#include "../ModText.h"
#include "../Online/NetSession.h"
#include "../System/PlayerInfo.h"
#include "../../LawnApp.h"
#include "../../Resources.h"
#include "graphics/Font.h"

OnlineStatusWidget::OnlineStatusWidget(LawnApp* theApp)
{
	mApp = theApp;

	// 没开会话（绝大多数时候）就整个藏起来：不画东西的 widget 只要还可见，
	// 它那块矩形照样会把点击吃掉。
	mVisible = false;
	Resize(8, 8, 10, 10);
}

OnlineStatusWidget::~OnlineStatusWidget()
{
}

void OnlineStatusWidget::Update()
{
    Widget::Update();auto* session=mApp->mOnlineSession;
    bool show=session&&session->IsActive();
    if (!show) mPressed = false;
    if(show&&!mVisible)mIpText=NetLink::GetLocalIPv4Text();
    SetVisible(show);if(show){Resize(mX,mY,330,52);MarkDirty();}
}

void OnlineStatusWidget::Draw(Graphics* g)
{
    auto* session=mApp->mOnlineSession;if(!session||!session->IsActive())return;
    Graphics stone(*g);stone.SetColorizeImages(false);
    stone.DrawImage(IMAGE_DIALOG_CENTERMIDDLE,Rect(0,0,mWidth,mHeight),Rect(0,0,IMAGE_DIALOG_CENTERMIDDLE->mWidth,IMAGE_DIALOG_CENTERMIDDLE->mHeight));
    stone.SetColor(Color(175,160,120));stone.DrawRect(0,0,mWidth-1,mHeight-1);
    std::string title=RoomPresentation::Summary(mApp);auto tag=GetTitleTagText();if(!tag.empty())title+=" · "+tag;
    SeatUi::Text(g,title,8,22,mWidth-16,18,Color(245,230,165));
    SeatUi::Text(g,RoomPresentation::CompactRoster(session),8,45,mWidth-16,18,Color(210,225,235));
}

void OnlineStatusWidget::MouseUp(int x, int y, int theClickCount)
{
	bool activate = mPressed && theClickCount == 1 && Rect(0, 0, mWidth, mHeight).Contains(x, y);
	mPressed = false;
	if (!activate) return;

	// 面板就是断开连接的入口，也是唯一能看到完整状态（本机 IP、失败原因）的地方
	mApp->PlaySample(Sexy::SOUND_TAP);
	mApp->DoOnlineDialog();
}

// 标题后缀：中继挂房间码（两边都挂——房主要念得出来，队友要核对进对没进对）；
// 直连才是老板子：主机念自己的 IP（队友要输进 Join 框的就是它），客户端那格留空。
std::string OnlineStatusWidget::GetTitleTagText()
{
	NetSession* aSession = mApp->mOnlineSession;
	if (!aSession) return "";
	if (aSession->IsRelay())
	{
		std::string aCode = aSession->GetRoomCode();
		if (aCode.empty()) return "";
		return ModText::Tr("房间 ", "Room ") + aCode;
	}
	if (aSession->GetRole() != NetSession::Role::HOST) return "";
	return mIpText;
}


void OnlineStatusWidget::MouseDown(int x, int y, int theClickCount)
{
	mPressed = theClickCount == 1 && Rect(0, 0, mWidth, mHeight).Contains(x, y);
}

void OnlineStatusWidget::LostFocus()
{
	mPressed = false;
	Widget::LostFocus();
}
