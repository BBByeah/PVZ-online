#include "OnlineDialog.h"
#include "RoomDirectoryDialog.h"
#include "RunHistoryDialog.h"
#include "OnlineSeatDialog.h"
#include "OnlineRoomLayout.h"
#include "CjkStoneButton.h"
#include "GameButton.h"
#include "../LawnCommon.h"
#include "../ModText.h"
#include "../Online/NetSession.h"
#include "../../LawnApp.h"
#include "../../Resources.h"
#include "graphics/Font.h"
#include "widget/WidgetManager.h"

// LawnCommon.cpp 里那张输入框配色表（CreateEditWidget 用的就是它）
extern int gLawnEditWidgetColors[][4];

namespace
{
	// EditListener::AllowChar 在这个框架里是**注释掉的**（EditListener.h 里那几条虚函数没开，
	// EditWidget::KeyChar 里调用它的那行也注释着），所以"这个输入框收哪些字符"挂不到 listener 上；
	// 每敲一个键必过的只剩 LawnEditWidget::KeyChar 这条虚函数——过滤只能挂在这儿。
	class OnlineEditWidget : public LawnEditWidget
	{
	public:
		enum Filter
		{
			FILTER_HOST,	// 地址：字母数字 + . -（IPv4 / 主机名）
			FILTER_CODE,
            FILTER_SERVER
		};

		OnlineEditWidget(int theId, EditListener* theListener, Dialog* theDialog, Filter theFilter)
			: LawnEditWidget(theId, theListener, theDialog), mFilter(theFilter)
		{
			// 地址和房间码都没有"首字母大写"这回事——地址给大写了就找不着主机
			mAutoCapFirstLetter = false;
		}

		virtual void KeyChar(char theChar)
		{
			if (mFilter == FILTER_CODE && theChar >= 'a' && theChar <= 'z')
				theChar = (char)(theChar - 'a' + 'A');
			if (!Allows(theChar)) return;
			LawnEditWidget::KeyChar(theChar);
		}

	private:
		bool Allows(char theChar) const
		{
			bool anAlnum = (theChar >= '0' && theChar <= '9')
				|| (theChar >= 'A' && theChar <= 'Z')
				|| (theChar >= 'a' && theChar <= 'z');
			if (mFilter == FILTER_CODE) return anAlnum || theChar=='-';
			return anAlnum || theChar == '.' || theChar == '-' || (mFilter==FILTER_SERVER&&theChar==':');
		}

		Filter mFilter;
	};

	LawnEditWidget* CreateOnlineEditWidget(int theId, EditListener* theListener, Dialog* theDialog,
		OnlineEditWidget::Filter theFilter)
	{
		OnlineEditWidget* aWidget = new OnlineEditWidget(theId, theListener, theDialog, theFilter);
		// 和 CreateEditWidget 同一套外观（字体/配色/光标闪烁），只是多了字符过滤
		aWidget->SetFont(Sexy::FONT_BRIANNETOD16);
		aWidget->SetColors(gLawnEditWidgetColors, EditWidget::NUM_COLORS);
		aWidget->mBlinkDelay = 14;
		return aWidget;
	}

    class RoomButton : public LawnStoneButton {
    public:
        RoomButton(int id,ButtonListener* listener):LawnStoneButton(nullptr,id,listener){}
        void Draw(Graphics* g) override {
            if(!mBtnNoDraw)SeatUi::Button(g,Rect(0,0,mWidth,mHeight),mLabel.c_str(),!mDisabled,mIsDown&&mIsOver);
        }
    };
    LawnStoneButton* MakeOnlineButton(int id,ButtonListener* listener,const char* label) {
        auto* button=new RoomButton(id,listener);button->SetLabel(label);
        button->mHasAlpha=true;button->mHasTransparencies=true;return button;
    }
}

OnlineDialog::OnlineDialog(LawnApp* theApp) :
	LawnDialog(theApp, Dialogs::DIALOG_ONLINE, true,
		_S(""), _S(""), _S(""), Dialog::BUTTONS_NONE)
{
	mApp = theApp;
	mVerticalCenterText = false;

	// @pvz-online: both languages use the shared readable stone layout.
	mHostButton = MakeOnlineButton(OnlineDialog::OnlineDialog_Host, this, ModText::Tr("建房", "Host"));
	mJoinButton = MakeOnlineButton(OnlineDialog::OnlineDialog_Join, this, ModText::Tr("加入", "Join"));
	mCloseButton = MakeOnlineButton(OnlineDialog::OnlineDialog_Close, this, ModText::Tr("返回", "Back"));
	mStartButton = MakeOnlineButton(OnlineDialog_Start,this,ModText::Tr("房主开始","Start run"));
	mDisconnectButton = MakeOnlineButton(OnlineDialog::OnlineDialog_Disconnect, this, ModText::Tr("断开", "Disconnect"));
	mSwapButton = MakeOnlineButton(OnlineDialog::OnlineDialog_Swap, this, ModText::Tr("队友", "Seats"));
	mAcceptButton = MakeOnlineButton(OnlineDialog::OnlineDialog_Accept, this, ModText::Tr("同意", "Accept"));
	mRejectButton = MakeOnlineButton(OnlineDialog::OnlineDialog_Reject, this, ModText::Tr("拒绝", "Reject"));
	mCreateRoomButton = MakeOnlineButton(OnlineDialog::OnlineDialog_CreateRoom, this, ModText::Tr("创建房间", "Create Room"));
	mJoinRoomButton = MakeOnlineButton(OnlineDialog::OnlineDialog_JoinRoom, this, ModText::Tr("加入房间", "Join Room"));

	// 直连那格的过滤和 Server 同一套（原来挂的 AllowChar 是死钩子，从来没被调过）
	mIpEditWidget = CreateOnlineEditWidget(OnlineDialog::OnlineDialog_IpEdit, this, this,
		OnlineEditWidget::FILTER_HOST);
	mIpEditWidget->mMaxChars = 253;			// IPv4 or DNS hostname
	mIpEditWidget->SetText(_S("127.0.0.1"), true);

	mCodeEditWidget = CreateOnlineEditWidget(OnlineDialog::OnlineDialog_CodeEdit, this, this,
		OnlineEditWidget::FILTER_CODE);
	mCodeEditWidget->mMaxChars = 7;

    mDirectoryButton=MakeOnlineButton(OnlineDialog_Directory,this,ModText::Tr("公开房间","Public rooms"));
    mHistoryButton=MakeOnlineButton(OnlineDialog_History,this,ModText::Tr("最近记录","Recent runs"));
    mVisibilityButton=MakeOnlineButton(OnlineDialog_Visibility,this,ModText::Tr("公开房间","Public room"));
    mLocalPageButton=MakeOnlineButton(OnlineDialog_LocalPage,this,ModText::Tr("单人与局域网游戏","Solo / LAN game"));
    mServerPageButton=MakeOnlineButton(OnlineDialog_ServerPage,this,ModText::Tr("在线房间","Online rooms"));
    mSoloButton=MakeOnlineButton(OnlineDialog_Solo,this,ModText::Tr("单人游戏","Play solo"));
    if(RoomPresentation::Active(mApp->mOnlineSession))mPage=mApp->mOnlineSession->IsRelay()?OnlineRoomLayout::SERVER:OnlineRoomLayout::LOCAL;
	mLocalIpLoaded = false;

	Resize(0,0,760,560);
}

OnlineDialog::~OnlineDialog()
{
    delete mLocalPageButton;delete mServerPageButton;delete mSoloButton;
	delete mDirectoryButton;delete mHistoryButton;delete mVisibilityButton;
	delete mHostButton;
	delete mJoinButton;
	delete mCloseButton;
	delete mDisconnectButton;
	delete mSwapButton;
	delete mAcceptButton;
	delete mRejectButton;
	delete mCreateRoomButton;
	delete mJoinRoomButton;
	delete mStartButton;
	delete mIpEditWidget;
	delete mCodeEditWidget;
}

void OnlineDialog::Resize(int x,int y,int width,int height)
{
    LawnDialog::Resize(x,y,width,height);
    using namespace OnlineRoomLayout;
    mLocalPageButton->Resize(LocalPage());mServerPageButton->Resize(ServerPage());mSoloButton->Resize(Solo());
    mDirectoryButton->Resize(40,480,260,46);mHistoryButton->Resize(552,34,168,42);mVisibilityButton->Resize(480,440,232,32);
    mHostButton->Resize(DirectHost());mJoinButton->Resize(DirectJoin());
    mCreateRoomButton->Resize(Create());mJoinRoomButton->Resize(Join());
    mCloseButton->Resize(Close());mDisconnectButton->Resize(Disconnect());
    mSwapButton->Resize(Seats());mStartButton->Resize(Start());
    mAcceptButton->Resize(Seats());mRejectButton->Resize(Disconnect());
    auto editRect=[](LawnEditWidget* edit,Rect r){edit->Resize(r.mX,r.mY,r.mWidth,r.mHeight);};
    editRect(mIpEditWidget,Address());editRect(mCodeEditWidget,Code());
}

void OnlineDialog::AddedToManager(WidgetManager* theWidgetManager)
{
	LawnDialog::AddedToManager(theWidgetManager);
    AddWidget(mLocalPageButton);AddWidget(mServerPageButton);AddWidget(mSoloButton);
	AddWidget(mDirectoryButton);AddWidget(mHistoryButton);AddWidget(mVisibilityButton);
	AddWidget(mHostButton);
	AddWidget(mJoinButton);
	AddWidget(mCloseButton);
	AddWidget(mDisconnectButton);
	AddWidget(mSwapButton);
	AddWidget(mAcceptButton);
	AddWidget(mRejectButton);
	AddWidget(mCreateRoomButton);
	AddWidget(mJoinRoomButton);
	AddWidget(mStartButton);
	AddWidget(mIpEditWidget);
	AddWidget(mCodeEditWidget);
    theWidgetManager->SetFocus(this);
    Update();
}

void OnlineDialog::RemovedFromManager(WidgetManager* theWidgetManager)
{
	LawnDialog::RemovedFromManager(theWidgetManager);
    RemoveWidget(mLocalPageButton);RemoveWidget(mServerPageButton);RemoveWidget(mSoloButton);
	RemoveWidget(mDirectoryButton);RemoveWidget(mHistoryButton);RemoveWidget(mVisibilityButton);
	RemoveWidget(mHostButton);
	RemoveWidget(mJoinButton);
	RemoveWidget(mCloseButton);
	RemoveWidget(mDisconnectButton);
	RemoveWidget(mSwapButton);
	RemoveWidget(mAcceptButton);
	RemoveWidget(mRejectButton);
	RemoveWidget(mCreateRoomButton);
	RemoveWidget(mJoinRoomButton);
	RemoveWidget(mStartButton);
	RemoveWidget(mIpEditWidget);
	RemoveWidget(mCodeEditWidget);
}

void OnlineDialog::Update()
{
	LawnDialog::Update();

	NetSession* aSession = mApp->mOnlineSession;
	// 死了的会话（掉线 / 版本不符被拒）按"没连着"对待：Host/Join 直接给回来，
	// 玩家不用先按一下 Disconnect 才能重开。死因不会因此丢掉——状态行照旧读
	// GetStatusText()，小条也还把短标签挂着，直到下一次 Start 把会话复位。
	// （ResetToOff 现在会把 NetLink 收干净，所以从这里重开局是通的，见 T19。）
    mAllocator.Update();
    if(mAllocator.state==RelayNodes::Allocator::READY){
        const auto endpoint=mAllocator.chosen;mAllocator.Cancel();
        if(mPage==OnlineRoomLayout::SERVER&&!RoomPresentation::Active(aSession))aSession->StartRoomHost(endpoint.host.c_str(),endpoint.port);
    }else if(mAllocator.state==RelayNodes::Allocator::UNAVAILABLE){
        mAllocator.Cancel();mServerInputError=ModText::Tr("暂时无法创建房间，请稍后重试。","Could not create a room. Please retry shortly.");
    }
    bool anActive=RoomPresentation::Active(aSession);
    const bool allocating=mAllocator.state==RelayNodes::Allocator::LOADING;

	// 中继：房间码一回来就写进 Code 框。房主那格是自己没有的（由服务器生成），
	// 面板上是"我在哪个房间"最直白的一处；按码加入的人也能看到自己的码归一化后的样子。
	// 只在码真的换了的时候写一次——每帧都写会把玩家正在框里敲的字擦掉。
	if (aSession && aSession->IsRelay())
	{
		const std::string& aCode = aSession->GetRoomCode();
        if (!aCode.empty() && aCode != mShownRoomCode)
		{
			mShownRoomCode = aCode;
            mCodeEditWidget->SetText(RelayNodes::ShareCode({aSession->GetRelayHost(),aSession->GetRelayPort()},aCode).c_str(), true);
		}
	}

    const bool local=!anActive&&mPage==OnlineRoomLayout::LOCAL;
    const bool server=!anActive&&mPage==OnlineRoomLayout::SERVER;
    mLocalPageButton->mVisible=!anActive;mServerPageButton->mVisible=!anActive;
    mSoloButton->mVisible=local||(anActive&&aSession->IsRelay()&&!mApp->mBoard);
    mSoloButton->Resize(anActive?Rect(40,440,420,32):OnlineRoomLayout::Solo());
    mSoloButton->SetLabel(mConfirmSolo?ModText::Tr("确认退房并单人开始","Confirm: leave and play solo"):
        anActive?ModText::Tr("返回单人游戏","Switch to solo"):ModText::Tr("单人游戏","Play solo"));
    mDirectoryButton->mVisible=server;mDirectoryButton->SetDisabled(!server||allocating);
    mVisibilityButton->mVisible=anActive&&aSession->IsRelay()&&aSession->IsHostSeat()&&!aSession->mDirectoryStarted;
    mVisibilityButton->SetLabel(aSession&&!aSession->mDirectoryConfirmed?ModText::Tr("正在更新…","Updating..."):
        aSession&&aSession->mDirectoryPublic?ModText::Tr("公开房间：开","Public: on"):ModText::Tr("公开房间：关","Public: off"));
    mHostButton->mVisible=local;mJoinButton->mVisible=local;
    mCreateRoomButton->mVisible=server;mJoinRoomButton->mVisible=server;
    mCreateRoomButton->SetDisabled(!server||allocating);mJoinRoomButton->SetDisabled(!server||allocating);
    mCreateRoomButton->SetLabel(allocating?ModText::Tr("正在连接…","Connecting..."):ModText::Tr("创建房间","Create room"));
    for(auto* edit:{mIpEditWidget,mCodeEditWidget}) {
        const bool visible=edit==mIpEditWidget?local:server;
        if(!visible&&edit->mHasFocus)mWidgetManager->SetFocus(this);
        edit->SetDisabled(!visible||allocating);edit->mVisible=visible;
    }
    mDisconnectButton->mVisible=anActive;mSwapButton->mVisible=anActive;
    mDisconnectButton->SetLabel(mApp->mBoard?ModText::Tr("保存并退房","Save and leave"):ModText::Tr("退出房间","Leave room"));
    mCloseButton->Resize(anActive?OnlineRoomLayout::Close():OnlineRoomLayout::Start());
    mStartButton->mVisible=anActive;
    const bool canStart=OnlineRoomLayout::CanStart(mApp);
    mStartButton->SetLabel(OnlineRoomLayout::NeedsPlayers(aSession)?ModText::Tr("等待队友加入","Wait for players"):ModText::Tr("房主开始","Start run"));
    mStartButton->SetDisabled(!canStart);
    mSwapButton->SetDisabled(!anActive);
    mAcceptButton->mVisible=false;mRejectButton->mVisible=false;
    if(!mLocalIpLoaded){mLocalIpText=NetLink::GetLocalIPv4Text();mLocalIpLoaded=true;}
    MarkDirty();
}

void OnlineDialog::Draw(Graphics* g)
{
    auto* session=mApp->mOnlineSession;
    std::string detail=session?session->GetStatusText():ModText::Tr("创建房间或输入房间码加入队友。","Create a room or enter a code to join your team.");
    if(session&&session->HasIncomingSwapRequest())detail=ModText::Tr("收到换位申请；打开席位查看并答复。","Swap request received; open Seats to reply.");
    else if(mApp->mOnlineWaitingStartAck)detail=RoomPresentation::WaitingSeats(session,true);
    else if(session&&session->IsConnected()&&!session->IsHostSeat())detail=ModText::Tr("等待房主选择模式并开始；可先查看席位。","Waiting for the host to choose a mode; you can view Seats.");
    OnlineRoomLayout::Draw(g,mApp,detail,mLocalIpText,mPage);
    if(!mServerInputError.empty()&&!RoomPresentation::Active(session))SeatUi::Text(g,mServerInputError,48,465,664,15,Color(255,175,120));
    if(mAllocator.state==RelayNodes::Allocator::LOADING)SeatUi::Text(g,ModText::Tr("正在寻找可用房间…","Finding an available room..."),48,465,664,15,Color(210,220,195));
    if(!RoomPresentation::Active(session)) {
        for(auto* edit:{mIpEditWidget,mCodeEditWidget})if(edit->mVisible)DrawEditBox(g,edit);
    }
}
void OnlineDialog::KeyDown(KeyCode key) {
    if(key==KEYCODE_ESCAPE)ButtonDepress(OnlineDialog_Close);
}

void OnlineDialog::ButtonDepress(int theId)
{
    if(theId==OnlineDialog_LocalPage||theId==OnlineDialog_ServerPage) {
        if(RoomPresentation::Active(mApp->mOnlineSession))return;
        mAllocator.Cancel();mServerInputError.clear();
        mPage=theId==OnlineDialog_LocalPage?OnlineRoomLayout::LOCAL:OnlineRoomLayout::SERVER;
        mConfirmSolo=false;Update();mWidgetManager->SetFocus(this);return;
    }
    if(theId==OnlineDialog_Solo) {
        if(mApp->mBoard||!mSoloButton->mVisible)return;
        if(RoomPresentation::Active(mApp->mOnlineSession)&&!mConfirmSolo){mConfirmSolo=true;Update();return;}
        auto* app=mApp;app->mOnlineSession->Close();app->mPendingEndlessScene=-1;app->mPendingAdventure=true;
        app->KillDialog(DIALOG_ONLINE);return;
    }
    mConfirmSolo=false;
	switch (theId)
	{
    case OnlineDialog_Directory:
        if(!RoomPresentation::Active(mApp->mOnlineSession)&&mPage==OnlineRoomLayout::SERVER&&mAllocator.state!=RelayNodes::Allocator::LOADING)
            mApp->AddDialog(DIALOG_ROOM_DIRECTORY,new RoomDirectoryDialog(mApp));
        return;
    case OnlineDialog_History:
        mApp->AddDialog(DIALOG_RUN_HISTORY,new RunHistoryDialog(mApp));return;
    case OnlineDialog_Visibility:
        if(mVisibilityButton->mVisible)mApp->mOnlineSession->SetDirectoryPublic(!mApp->mOnlineSession->mDirectoryPublic);
        return;
	case OnlineDialog_Start:
		if(!OnlineRoomLayout::CanStart(mApp))return;
		{auto* app=mApp;app->KillDialog(DIALOG_ONLINE);app->RequestAdventure();}
		return;
	case OnlineDialog::OnlineDialog_Host:
		StartHost();
		break;

	case OnlineDialog::OnlineDialog_Join:
		StartJoin();
		break;

	case OnlineDialog::OnlineDialog_Close:
		// 关面板不动连接：连接活在会话里，面板只是遥控器
		mApp->KillDialog(Dialogs::DIALOG_ONLINE);
		break;

	case OnlineDialog::OnlineDialog_Disconnect:
        if(mApp->mBoard){
            auto* app=mApp;app->DoBackToMain(false);
            if(!app->mBoard){app->mOnlineSession->Close();app->KillDialog(DIALOG_ONLINE);}
            return;
        }
		if (mApp->mOnlineSession)
		{
			mApp->mOnlineSession->Close();
		}
		break;

	case OnlineDialog::OnlineDialog_Swap:
		// 2026-10-15：改开选人框——任选上座队友（含预览），原「只跟环上后一位」的直发退役。
		// 请求/答复/超时仍在 NetSession；战斗可查看，席位页禁止换位。
		if (mApp->mOnlineSession != nullptr)
		{
			mApp->AddDialog(DIALOG_ONLINE_SWAP, new OnlineSeatDialog(mApp));
		}
		break;

	case OnlineDialog::OnlineDialog_Accept:
		if (mApp->mOnlineSession)
		{
			mApp->mOnlineSession->AnswerSwapRequest(true);
		}
		break;

	case OnlineDialog::OnlineDialog_Reject:
		if (mApp->mOnlineSession)
		{
			mApp->mOnlineSession->AnswerSwapRequest(false);
		}
		break;

	case OnlineDialog::OnlineDialog_CreateRoom:
		StartRoomHost();
		break;

	case OnlineDialog::OnlineDialog_JoinRoom:
		StartRoomJoin();
		break;
	}
}

// 回车 = 这一格对应的那把键：地址框回车走直连 Join（老行为），房间码框回车走 Join Room。
// 服务器地址框回车也走 Join Room——输完地址顺手回车是最顺的动作，此时码多半也填好了。
void OnlineDialog::EditWidgetText(int theId, const SexyString& theString)
{
	(void)theString;
	switch (theId)
	{
	case OnlineDialog::OnlineDialog_ServerEdit:
	case OnlineDialog::OnlineDialog_CodeEdit:
		StartRoomJoin();
		break;

	case OnlineDialog::OnlineDialog_IpEdit:
	default:
		StartJoin();
		break;
	}
}

std::string OnlineDialog::GetIpText()
{
	std::string anIp = mIpEditWidget->mString;
	if (anIp.empty()) anIp = "127.0.0.1";
	return anIp;
}

std::string OnlineDialog::GetRoomCodeText()
{
	return mCodeEditWidget->mString;
}

void OnlineDialog::StartHost()
{
    if(mPage!=OnlineRoomLayout::LOCAL)return;
	if(RoomPresentation::Active(mApp->mOnlineSession))return;
	if (!mApp->mOnlineSession) return;
	mApp->mOnlineSession->StartHost();
}

void OnlineDialog::StartJoin()
{
    if(mPage!=OnlineRoomLayout::LOCAL)return;
	if(RoomPresentation::Active(mApp->mOnlineSession))return;
	if (!mApp->mOnlineSession) return;
	mApp->mOnlineSession->StartJoin(GetIpText().c_str());
}

void OnlineDialog::StartRoomHost()
{
    if(mPage!=OnlineRoomLayout::SERVER||RoomPresentation::Active(mApp->mOnlineSession)||!mApp->mOnlineSession)return;
    if(mAllocator.state==RelayNodes::Allocator::LOADING)return;
    mShownRoomCode.clear();mServerInputError.clear();mAllocator.Start();
    mWidgetManager->SetFocus(this);Update();
}

void OnlineDialog::StartRoomJoin()
{
    if(mPage!=OnlineRoomLayout::SERVER||RoomPresentation::Active(mApp->mOnlineSession)||!mApp->mOnlineSession||mAllocator.state==RelayNodes::Allocator::LOADING)return;
    RelayNodes::Endpoint target;std::string code;
    // Four-character codes from old clients retain their original destination.
    if(!RelayNodes::ResolveCode(GetRoomCodeText(),RelayNodes::Catalog().front().endpoint,target,code)) {
        mServerInputError=ModText::Tr("请填写队友显示的完整房间码，例如 2-ABCD。","Enter your teammate's full room code, for example 2-ABCD.");return;
    }
    mShownRoomCode.clear();mServerInputError.clear();
    mApp->mOnlineSession->StartRoomJoin(target.host.c_str(),code.c_str(),target.port);
    mWidgetManager->SetFocus(this);
}
