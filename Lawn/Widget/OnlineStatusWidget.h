#ifndef __ONLINESTATUSWIDGET_H__
#define __ONLINESTATUSWIDGET_H__

#include <string>
#include "widget/Widget.h"

class LawnApp;

using namespace Sexy;

class OnlineStatusWidget : public Widget
{
public:
	LawnApp*			mApp;
	// 本机 IP：会话开始可见时算一次就存着——它要枚举网卡，不是每帧该干的活，
	// 而会话开着这段时间地址也不会变。
	std::string			mIpText;

public:
	OnlineStatusWidget(LawnApp* theApp);
	virtual ~OnlineStatusWidget();

	virtual void		Update();
	virtual void		Draw(Graphics* g);
	virtual void MouseDown(int x, int y, int theClickCount);
	virtual void LostFocus();
	virtual void		MouseUp(int x, int y, int theClickCount);

private:
	// 标题后面挂的那串字：中继 = 房间码（房主念给朋友）；直连 = 主机念自己的 IP，
	// 客户端这行留空（念自己的地址没有用）。
	std::string			GetTitleTagText();
	bool mPressed = false;
};

#endif
