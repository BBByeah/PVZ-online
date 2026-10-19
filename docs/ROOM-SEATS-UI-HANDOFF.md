# 双端联机状态与席位页（2026-10-15）

当前仍为 Build53 / Protocol1。PC 固定来源 `1ae3ec306b2daa8b602f61635e459c0511a322ee`，主 UI 提交 `8a0ac05`。不改中继、协议、玩法数值或换位事务；`onlineGameReady=false`。

## 已实现

- 两端房间入口「换位」改为「席位」。六席两列卡片显示名称、你、房主、空席、断线保留、刷怪倍率、额外开局阳光和漏怪去向；顶部显示实际接力链。房主身份跟随玩家，与首席、末席分开。
- 点击队友预览换位后倍率与额外阳光，再申请；对方同意才交换。页面保留申请、答复及超时反馈，支持收到请求后同意/拒绝。重复申请、空席和离线目标不可操作。
- 战斗和开局等待期间可查看，不允许发起或接受换位。未新增关间战斗换位能力。
- 首页两行状态条显示连接/版本差异、房间码、你与房主席位、人数，点击回房间。开局等待、收到申请及等待答复优先提示。
- 移动端复用原游戏石材资源，800×600逻辑布局，卡片344×90、主按钮160×46，按下与抬起须命中同一动作；接力顺序和倍率直接读取真实会话及现有规则。

核心文件：`Lawn/Online/RoomPresentation.h`、`Lawn/Widget/OnlineSeatDialog.h` 在两端内容相同。各自 OnlineDialog 接入；PC OnlineStatusWidget、Android GameSelector 接入首页状态条。后续改动请保持共享头文件同步。

## 定点验证与范围

- PC Release、Android Native/Release APK通过；v2签名和16KB zipalign通过，版本元数据一致。
- 本地中继、用户指定的云中继各通过12项真实会话检查：六席、真实倍率、末席无后继、空席跳过、编号、换位可用性、重复申请、房主移动后身份、已提交席位对应规则、战斗和开局只读。
- MuMu Android15经云中继加入六人房，触摸申请换位从P2到P1成功，房主仍P6，倍率/阳光/接力同步更新。最终包重新安装，六席画面见仓库外 `android-artifacts/room-ui/seats-final.png`。`seats-cloud-before-final.png`为调整背景前的换位后画面；其他中间截图仅用于定位输入或连接问题，不作为最终验收。
- 未重跑完整双端战斗、实际六名人类玩家、全部英文长名与重连运行画面。相应限制仍保留，不因此宣布全部联机验收完成。
- 测试后原5个用户文件恢复且SHA256逐个一致；没有新增userdata文件；应用已停止。本轮临时会话和本地中继进程已停止，未留下ADB转发。未推送远端。
- 原有 RunBuffs.cpp/.h 工作区改动保留，规范化补丁内容与此前快照相同，未计入本批提交；构建包含已有火力数值和文案变更（+30%→+20%，不只是文案；后续已由1b74abd单独提交）。

## 产物

- Android：`../android-artifacts/pvz-online-build53-room-ui-arm64.apk`，SHA256 `B61B1C5215EA9BC550B3B3B9E9A4292DDD9D1AD2D93E614A6177AC4282469BF2`，已安装MuMu。
- PC：`../runtime/PvZ-online-room-ui.exe`（同目录有游戏资源，可直接启动）；另存 `../android-artifacts/room-ui/PvZ-PC-Build53-room-ui.exe`，SHA256 `77CCF8F4F8F38A38B0CE06F651E586E72C06E38BB9C099D0913F55D8A77B4A33`。旧 runtime/SexyAppFramework.exe 正在运行，因此未覆盖；先关闭旧窗口，再启动新版。
- 构建、定点会话、签名、恢复记录均在仓库外 `android-artifacts/room-ui/`。
