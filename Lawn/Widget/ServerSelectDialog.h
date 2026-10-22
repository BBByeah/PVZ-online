#pragma once
#include "LawnDialog.h"
#include "OnlineUiDraw.h"
#include "../Online/RelayNodes.h"
#include <functional>
#include <memory>

class ServerSelectDialog : public LawnDialog {
    LawnApp* app;
    std::function<void(const RelayNodes::Endpoint&)> choose;
    std::vector<std::unique_ptr<RelayNodes::Probe>> probes;
    int selected=-1,down=-1;
    std::chrono::steady_clock::time_point last{};
    static Rect Row(int i){return Rect(48,164+i*120,704,104);}
    static Rect Back(){return Rect(48,524,168,46);}
    static Rect Refresh(){return Rect(304,524,168,46);}
    static Rect Select(){return Rect(584,524,168,46);}
    bool CanRefresh()const{return std::chrono::steady_clock::now()-last>=std::chrono::seconds(2);}
    void Fetch(){if(!CanRefresh())return;last=std::chrono::steady_clock::now();for(size_t i=0;i<probes.size();++i)probes[i]->Refresh(RelayNodes::Catalog()[i].endpoint);}
    int Hit(int x,int y)const {
        if(Back().Contains(x,y))return 100;
        if(Refresh().Contains(x,y)&&CanRefresh())return 101;
        if(Select().Contains(x,y)&&selected>=0)return 102;
        for(int i=0;i<int(probes.size());++i)if(Row(i).Contains(x,y))return i;
        return -1;
    }
public:
    ServerSelectDialog(LawnApp* a,const RelayNodes::Endpoint& current,std::function<void(const RelayNodes::Endpoint&)> callback):
        LawnDialog(a,DIALOG_SERVER_SELECT,true,"","","",Dialog::BUTTONS_NONE),app(a),choose(std::move(callback)) {
        Resize(0,0,800,600);
        for(const auto& node:RelayNodes::Catalog()) {
            if(RelayNodes::Same(node.endpoint,current))selected=int(probes.size());
            probes.emplace_back(new RelayNodes::Probe);
        }
        Fetch();
    }
    void Update()override{LawnDialog::Update();for(auto& probe:probes)probe->Update();MarkDirty();}
    void Draw(Graphics* g)override {
        SeatUi::Panel(g,Rect(0,0,800,600));
        SeatUi::Text(g,ModText::Tr("选择服务器","Choose a server"),48,76,704,29,Color(245,230,165),true);
        SeatUi::Paragraph(g,ModText::Tr("和朋友选择同一台服务器。分享房间时，请一并发送服务器编号。","Choose the same server as your friends. Share its number with your room code."),Rect(48,98,704,48),18,Color(205,220,225));
        for(int i=0;i<int(probes.size());++i) {
            auto row=Row(i);const auto& node=RelayNodes::Catalog()[i];const auto& probe=*probes[i];
            SeatUi::Strip(g,row);if(selected==i){g->SetColor(Color(225,205,100));g->DrawRect(row.mX+1,row.mY+1,row.mWidth-3,row.mHeight-3);}
            SeatUi::Text(g,ModText::IsChinese()?node.zh:node.en,row.mX+14,row.mY+30,360,23,Color(245,230,165));
            std::string status=probe.state==RelayNodes::Probe::LOADING?ModText::Tr("正在查询…","Checking..."):
                probe.state==RelayNodes::Probe::READY||probe.state==RelayNodes::Probe::LEGACY?ModText::Tr("在线","Online"):
                ModText::Tr("暂时无法查询","Status unavailable");
            SeatUi::Text(g,status,row.mX+390,row.mY+30,296,20,Color(205,220,225));
            const auto& c=probe.capacity;
            const std::string detail=probe.state==RelayNodes::Probe::READY?
                ModText::Tr("房间 ","Rooms ")+std::to_string(c.rooms)+"/"+std::to_string(c.roomLimit)+"  ·  "+std::to_string(c.players)+ModText::Tr(" 人在线"," players")+"  ·  "+std::to_string(probe.latencyMs)+" ms":
                probe.state==RelayNodes::Probe::LEGACY?ModText::Tr("该节点暂不提供人数统计","This node does not provide capacity statistics"):
                ModText::Tr("仍可选择后重试连接","You can select this server and try connecting");
            SeatUi::Text(g,detail,row.mX+14,row.mY+61,676,17,Color(205,220,225));
            SeatUi::Text(g,node.endpoint.Text(),row.mX+14,row.mY+88,676,16,Color(155,175,180));
        }
        SeatUi::Paragraph(g,ModText::Tr("普通四位房间码只在所选服务器内加入；例如 2-ABCD 会直接加入服务器 2。","A four-character code joins the selected server. A code such as 2-ABCD goes directly to Server 2."),Rect(48,432,704,68),18,Color(205,220,225));
        SeatUi::Button(g,Back(),ModText::Tr("返回","Back"),true,down==100);
        SeatUi::Button(g,Refresh(),ModText::Tr("刷新","Refresh"),CanRefresh(),down==101);
        SeatUi::Button(g,Select(),ModText::Tr("使用此服务器","Use server"),selected>=0,down==102);
    }
    void MouseDown(int x,int y,int clicks)override{down=clicks>0?Hit(x,y):-1;MarkDirty();}
    void MouseUp(int x,int y,int clicks)override {
        const int hit=Hit(x,y),pressed=down;down=-1;if(clicks<=0||hit<0||hit!=pressed)return;
        if(hit==100){app->KillDialog(DIALOG_SERVER_SELECT);return;}
        if(hit==101){Fetch();return;}
        if(hit==102&&selected>=0){choose(RelayNodes::Catalog()[selected].endpoint);app->KillDialog(DIALOG_SERVER_SELECT);return;}
        if(hit<int(probes.size()))selected=hit;
    }
    void KeyDown(KeyCode key)override{if(key==KEYCODE_ESCAPE)app->KillDialog(DIALOG_SERVER_SELECT);}
};
