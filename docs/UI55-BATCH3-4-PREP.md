# 双端 UI 第三、四批准备：现状盘点与开工清单

> 接续更新（2026-10-16）：第三批已进入实现与有限验收，当前事实见 [第三批状态](UI55-BATCH3-STATUS.md)。下文保留开工前盘点；第四批已进入阶段交付，当前事实见 [第四批状态](UI55-BATCH4-STATUS.md)。划取开关最终选用暂停中的辅助设置，不再沿用§6的高级选项默认候选。

2026-10-16。一、二批已完成（[实施交接](UI55-IMPLEMENTATION-STATUS.md)、[第二批交接](UI55-BATCH2-STATUS.md)）；本文件是第三批（词条/图鉴/设置）、第四批（主菜单/模式/奖励/结算）的开工前盘点。全部事实来自当前源码：PC `2e3b72e` / Android `73ae4b5`，Build55 / Protocol1 / 检查点v17 / `onlineGameReady=false`。本文只盘点不改代码、不构建不运行；总范围与遮挡/步骤预算见 [双端UI总计划](UI-REDESIGN-PLAN-BUILD55.md)。路径无前缀者为 PC 仓库相对路径，Android 对应物在 `src/` 下同名目录。

## 1. 范围与缺口对照

| 项 | PC 现状 | Android 现状 | 缺口编号 |
|---|---|---|---|
| 本局词条查看器 | 有基础待重做（§2.1） | 全缺：无读模型无入口 | F04 |
| 图鉴「可获得词条」 | 附录已按池过滤+三档排版，核对即可 | 有但藏底部切换钮，需对齐 PC 默认附词条 | F07 |
| 暂停/设置分组 | 单屏 8 控件无分组（§2.3） | 分叉版：语言循环钮、无硬件加速项；无血量/划取设置 | — |
| 触摸血量开关 | 不需要（H/J 键已有） | 数字绘制已有，缺触摸开关 | F06 余项 |
| 划取阳光 | 完整功能（§2.4），本批基本不动 | 全缺（含本机开关），本批独立提交补齐 | F03 |
| 主菜单/模式/三选一/投票/钱包 | 见 §3 | 见 §3 | — |
| 通关结算 | 奖杯屏已有（参照） | run 通关只有消息框 | F10 |

划界：F05 坚果保龄球配套、F12 额外玩法冒险门槛是独立功能任务，不混入 UI 批次（Android 仓库 `docs/UI-UPGRADE-PLAN-BUILD55.md` §4 已定）；`2eb8646` 植物三选一跨屏冷却未迁 Android，属规则迁移（走 PC-SYNC-WORKFLOW），在 §6 拍板归属。

## 2. 第三批页面现状

### 2.1 本局词条查看器

**PC（`Lawn/Widget/NewOptionsDialog.cpp:630-960`，重做对象）**
- 入口：仅闯关局（`RunInfoAvailable()` :630-634 = 非 `mFromGameSelector` 且 `IsRunMode()` 且 `mRunState!=NULL`）；暂停面板右侧竖带按钮（`RunInfoEntryRect` :636-646，y=258），点击 `OpenRunInfo()` :673-694。暂停面板本体 = `DIALOG_NEWOPTIONS`，由右上 Menu 键（Board.cpp:5330-5343）、ESC（Board.cpp:9864-9874）、队友暂停同步（LawnApp.cpp:1264-1311）打开。
- 结构：不开新对话框，覆盖层盖住 8 控件、关闭还原（CloseRunInfo :696-708）。单页从上到下：标题 → 模式/关数/难度行（:795-813）→【全局增益】→【僵尸强化】（全 0 整段隐藏 :852-873）→【单株强化】；**分页只作用单株区**，页数绘制时现算（:885-895）。翻页三路：PgUp/PgDn/←/→（:490-505）、滚轮（:753-764）、页脚三角 34px 命中（`RunInfoArrowRect` :662-669）。
- **重做要点**（总计划要求 vs 现状）：
  1. 显示只有「名称×层数」——无说明文字、无搜索输入；截名保「×N」（`NewOptionsDrawCjkFit` :104-130）。
  2. 单株区只遍历永久 `mBuffs`，**借入字段（`mLoanPlant/mLoanLevel/mLentLevel/mLoanBuffs`，RunState.h:176-178）完全未展示**。
  3. 全局段读原始 `GetBuffCount`（:831）、单株读 `mBuffs[i].mCount`（:911），**未用 `RunState::GetEffectiveBuffCount`**（RunState.cpp:1487-1501 = 永久 + 同关借入取 max + 三组跨表联动：寒冰菇/霜穿透、玉米加农炮/黄油两词条、坚果/尖刺/坚果保龄）。实际生效层数须改读同一模型；现有消费者口径见 LawnApp.cpp:3008-3144 与 CoopPlayDialog.h:48「→ 生效 N」。
- 画布占用：对话框按 `IMAGE_OPTIONS_MENUBACK` 居中（zh 覆盖包 423×498），面板 = 内缩 14px（:779-786）。

**Android：全缺。** `src/Lawn/Widget/NewOptionsDialog.cpp`（437 行）无对应状态/绘制，也无触摸替代入口；`src/` 全库仅 CoopPlay.cpp:166/176 借用提示文案与图鉴「可获得词条」页涉及词条展示。迁入时读模型用同源 `src/Lawn/Run/RunState`（`GetEffectiveBuffCount` 双端一致），UI 用 MobileCoopDraw 件，入口对齐 PC（暂停面板内）。

### 2.2 图鉴

- **PC（`Lawn/Widget/Almanac.cpp`，946 行）**：三页 INDEX/PLANTS/ZOMBIES（SetPage :420-456）。词条附录已按「可获得」过滤（`AlmanacRunEntries` :120-129 只收 `RunPlantUpgradeInPool`/`RunPlantBuff2InPool` 为真，RunBuffs.h:186-188/281-283），三档自适应排版（:197-270，死线 `ALMANAC_ENTRY_BOTTOM=519`）+ 单株翻页（`AlmanacPagerArrowRect` :144-150 命中 20px，仅鼠标，换株回页 0 :388）。植物说明/费用/冷却 = Rect(485,309,258,230)/(485,520,134,50)/(600,520,139,50)（:598-626）。**中文双路径**：原版说明/费用/冷却走字符串表+位图字体（pvz_zh.pak 字库）；词条附录走 ModText 宽字符。本批 PC 侧=核对口径与排版，无缺口。
- **Android（`src/Lawn/Widget/AlmanacDialog.cpp`，674 行，分叉重写）**：词条数据齐（`MobileAlmanacEntries` :346 按植物过滤池内+星级），但默认回说明页（换植物 :638）、要看词条必须点底部「说明 / 词条 N/M」切换钮（`kAlmanacEntryButton` Rect(222,554,430,42) :42，翻页 :623-627）——F07。改法方向已定（总计划：优先对齐 PC「介绍后附词条」），细节待第三批方案稿。
- 入口（双端）：暂停面板图鉴钮、主菜单、选卡界面（PC 另有AwardScreen.cpp:577）。Android `CanShowAlmanac` 不要求冒险进度（src LawnApp.cpp:2593-2600）。

### 2.3 暂停与设置

- **PC（NewOptionsDialog 单屏）**：8 控件（图鉴/重开/主菜单/回游戏 + 音乐/音效滑条 + 全屏/3D 加速，.h:22-41）+ 两块自绘（左下语言三档+「重启后生效」:327-375、右侧本局词条入口）；联机藏重开（:216-219）；**无分组无分页**。暂停语义：`Board::Pause`（Board.cpp:5388-5405）；队友暂停顶部横幅（:379-383）。血量键 = **H 僵尸 / J 植物**（非 1/2，Board.cpp:9826-9840），文件级 static，仅本机、不进协议、跨关不重置。简化暂停框 `DIALOG_PAUSED` 入口只剩空格+单机失焦（LawnApp.cpp:439-451/3527-3545）。
- **Android（分叉）**：硬件加速项位置放语言循环 `MobileStoneButton`（NewOptionsDialog.cpp:83-85，三语言+重启提示 :341-357）；联机暂停同步在 `src/Lawn/OnlineGame.cpp:598`。**无血量开关、无划取开关**（H/J 键盘切换已有：Board.cpp:8214-8221，数字绘制 :8020-8031）。
- 本批要做：双端「语言、血量、本局词条、协作入口、既有选项」有序分组（总计划 §3 暂停与设置行）；Android 补血量触摸开关（F06 余项，挂 NewOptionsDialog，复用既有 static 开关，不改实体数值）。

### 2.4 划取阳光（F03，独立提交）

- **PC 已完整**：`Board::MouseDrag`（Board.cpp:3620-3642）闸 = `mApp->mSlideCollect`（注册表 `ModSlideCollect` **默认开**，LawnApp.cpp:180-184），`IsSun` 闸 + `Collect` 防重；开关 UI = 高级选项第2页「滑动收阳光」（RunOptionsDialog.h:45），确定落注册表并热改（RunModeDialog.cpp:423-426）；本机设置不进联机载荷。点击收集链：Board.cpp:5099→5164-5173→Coin.cpp:1401-1419→`Collect`（:1058-1065）；关底自动收 Coin.cpp:1033-1054。
- **Android 全缺**：`src/Lawn/Board.cpp` MouseDown（:4807）→Coin 点击收集已有；MouseDrag（:3374-3385）只有协作拦截+拖种 8px 判定；无划取、无开关、无持久化。
- 独立提交边界（沿用审计第二批方案）：复用 PC 规则；补本机开关+保存；只在未被持卡/工具/弹窗/观战捕获的草坪滑动收真实阳光，排除 `mVisualOnly` 展示阳光；保留拖种/观战/面板捕获优先级。**不碰 PC 侧 Coin.cpp（混合编码，见 §5）**。

## 3. 第四批页面现状

### 3.1 主菜单
- **PC（GameSelector.cpp，1663 行）**：按钮 id 全集 GameSelector.h:31-50；主位大墓碑=组队闯关（:1534-1543 → `RequestAdventure` LawnApp.cpp:1682-1725）、第三槽 PUZZLE 石板=原版战役（:1416-1427，用户 2026-10-02 拍板注）、Survival 槽=无尽（:1483-1486）、商店/图鉴/花园/选项/帮助/退出/换用户/成就各有归属（:1487-1530）；已隐藏 Minigame/Puzzle/Zombatar/QuickPlay，PvP 灰置占位（:289-302）。房间状态条 `OnlineStatusWidget` @(8,8)，激活 330×52 两行（Summary+CompactRoster，OnlineStatusWidget.cpp:81-92），点击回联机面板。**续局无独立入口**：主位排队后弹「继续闯关？」框（LawnApp.cpp:1852-1870）。死代码：`GetSeatLine/GetStateLine`（OnlineStatusWidget.cpp:127-231）无调用方，头注释（.h:20-24）旧口径。
- **Android（GameSelector.cpp，1447 行）**：原版墓碑保留 + 加桩「闯关」5001 /「联机」5002 石材小钮（y330）+ `MobileRoomStatus` 条 5003（:60-80/:104-120/:112）。原版冒险入口易误入（审计 §5）——导航含义澄清是本批内容，不删原版模式。

### 3.2 模式与高级选项
- **PC**：`RunModeDialog`（431 行）三时长卡 104×115/pitch155 + 缩略图（:323）+ 出怪难度三档（仅联机主机或无尽显示 :74）+ 高级选项按钮；无尽变体撤卡改锁环境名（:337-345）。`RunOptionsDialog`（513 行）九行三页环绕翻页（非当前页 Resize -2000 :288-346）：页1 规模4/节奏3/植物僵尸2；页2 滑动收阳光/Boss3/增益倍选3；页3 移速3/僵尸buff3/场景阶梯5（无尽隐藏行8）。**硬约束：构造实测总高 614>600，已钳到 `mHeight-8`（:253-263），余量为 0——重排必须重排信息组织，不能加行**（总计划 §1 规则）。开局流：点卡→`StartRun`→`BeginStartPicks`（非无尽=4植物+2增益六次三选一；无尽=4增益+原版选卡，LawnApp.cpp:1960-1962）。
- **Android（`src/Lawn/Run/RunFlowScreen.cpp`，261 行，对应物）**：单屏双页（mModePage）——25/10/5 卡+无尽+难度+6 选项钮+僵尸强化+阶梯+倍选+移速+继续+选项翻页（:92-109/:204），StartRun 传参 :244-245。视觉仍是纯绿底矩形钮（UI-STYLE-AUDIT P1），本批按同一视觉规范整理，不重写状态机。

### 3.3 三选一 / 投票 / 钱包
- **PC `RunPickDialog`（413 行）**：植物卡=种子包图（:304-311）+稀有度星标列顶（:339-362）+「已有 x/N」（:366-377）；重摇恒付费（底价10、同屏每摇×10、券态、钱不够置灰 :103-120）+「放弃」（:244-247）；钱罐 (57,599-H) 与局内同口径（:285-300 ↔ Board.cpp:8448-8473）；左上协作入口叠层（20,16,140,40）+B 键（:404-413）。无翻页/冷却控件（跨屏冷却为纯内存规则）。打开/收口在 LawnApp.cpp:2329-2480/2487/2520/2864。**宽度/高度已封顶**（宽580/列220/高500，:157-194，中文 6 行说明时「已有」行压字的历史遗留可顺带核）。
- **PC `ZombieVoteDialog`（229 行）**：三候选+弃权+状态行（倒计时+已投 :168）；一票锁全钮（:211-219）；30 秒超时弃权；换关两处门口打开（LawnApp.cpp:2348/2444）。协议 MSG 20/21/22 不动。
- **Android**：同域功能在 `RunFlowScreen` pick 页（`RunChoiceCard` :38-70 种子卡/星级/层数、重摇 :123-125、跳过 :126、Take :253-257）；F01/F02 退出闭环已修（id==9 → 保存 → `DoBackToMain(true)` :190-200）；`ZombieVoteDialog.cpp`（59 行）三候选+弃权 MobileStoneButton；钱包=棋盘 CoinBank（Board.cpp:7757-7770）+ pick 页（RunFlowScreen.cpp:178-181）。本批=信息顺序/操作区对齐 PC 口径（同一信息顺序、大点击区，总计划 §3 奖励行），不重写已有事务。

### 3.4 结算
- **PC（参照实现）**：通关奖杯屏 `AwardScreen` `AWARD_RUN_COMPLETE`（ConstEnums.h:88），`DrawRunComplete` :290-314（奖杯+三行文案中英两版+「回主菜单」宽字符钮 :196-200/:469-475）；调用点 `UpdateOnlineEnd`（LawnApp.cpp:1549-1557）与单机 `UpdateRunEnd`（:2250-2256）。失败=`GameOverDialog`（LawnDialog.cpp:445-570，联机客户端藏 Try Again；闯关=「重试本关」）；断线=退主菜单+中文框（LawnApp.cpp:1352-1393）。
- **Android 缺 run 通关反馈（F10）**：`src/Lawn/LawnApp.cpp:1545-1551` 与 `src/Lawn/OnlineGame.cpp:882` 通关只弹「闯关完成」消息回主菜单；`AwardScreen.cpp`（657 行）只有原版冒险分支（:321-343/:408-409）。本批补 `AWARD_RUN_COMPLETE` 对齐，不重复发奖励、不改成长规则。

### 3.5 商店/花园（只查不改）
PC `StoreScreen.cpp` 只放行 ZEN1/ZEN2 两页（:813-820）；`ZenGarden.cpp`（2479 行）原版场景保留。Android `StoreScreen.cpp`（1190 行，升级页 ≥42 关 :235-274）、`ZenGarden.cpp`（2464 行，共同两页开放）。本批只做中文裁切/按钮/返回一致性检查，原版主体与购买规则不动；F12 门槛（小游戏/解谜/生存/ChallengeScreen 奖杯链、Android 商店升级条件）不在此批。

## 4. 可复用基础与共享基线

- `OnlineUiDraw.h`（namespace SeatUi，99 行，双端字节一致）：`Width/Fit/Text/Paragraph`（宽字符量宽/UTF-8 感知截断/单行/段落折行）、`Panel`（九宫石材）、`Strip`、`Button`（三段石钮+按下+置灰+22px 标签）。**NewOptionsDialog 与 Almanac（双端）现状未复用它**（手绘 FillRect + 文件内 static CJK 助手，如 NewOptionsDialog.cpp:86-130）——第三批重做时统一到 SeatUi 是自然路径。
- 翻页几何先例（绘制/命中/光标共用同一份几何）：`RunInfoArrowRect`（34px）、`AlmanacPagerArrowRect`（20px+页码）。
- 按钮：PC `CjkStoneButton`；Android `MobileCoopDraw.h` 的 `MobileStoneButton`。44×44 起步命中区 + 按下/松开同动作路由先例在 `CoopHud.h`（含 `#ifdef __ANDROID__` 挂点 :12-18/:78-99）。
- Android 输入：无自研手指路由——SDL2 touch 合成鼠标 → `WidgetManager::RemapMouse` 线性映射 800×600（WidgetManager.cpp:215-218）；触摸开关/划取直接走既有鼠标事件路径，无需新框架。
- 共享头基线（已 cmp 字节一致）：CoopHud.h(178)、CoopPlayDialog.h(147)、OnlineUiDraw.h(99)、OnlineRoomLayout.h(48)、OnlineReadyDialog.h、OnlineSeatDialog.h。同步无自动脚本，走 PC-SYNC-WORKFLOW（Android 仓库 `docs/PC-SYNC-WORKFLOW.md`：固定 PC SHA→只读审计→手工迁→`generate-port-version.ps1 -Check` 构建强制）。
- 文案纪律：位图字体路径不认 Tr 中文——石材钮/位图场景必须走 `CjkStoneButton`/ModText 宽字符（RunState.h:420-423 注）；新中文文案先查渲染路径。

## 5. 风险与边界

1. **GBK/混合编码文件**（批内尽量不碰）：PC `Coin.cpp`（3 行 GBK 注释的混合编码；划取阳光 Android 侧实现不要求动它）、`ContinueDialog.cpp`/`UserDialog.cpp`/`NewUserDialog.cpp`（换用户/续局弹窗，主菜单批若触及）、`Cutscene.h`、`LawnMower.cpp`、`ImitaterDialog.cpp`、`System/DataSync.*`、`System/ProfileMgr.cpp`、`Zombatar*`。第四批主路径文件（GameSelector/Run 三 Dialog/RunPick/ZombieVote/AwardScreen/Store/OnlineUiDraw/CjkStoneButton/ModText/LawnApp/Board）已验证全 UTF-8。
2. **画布硬约束**：RunOptionsDialog 高度余量为 0（§3.2）；`OnlineStartDialog` 正文每页 ≤8 行（8 行=614px>600，LawnApp.cpp:1897-1898）；RunPickDialog 宽/高已封顶（§3.3）。这三个页面的改动先做信息组织设计再动手。
3. **协议/数值冻结**：不改玩法数值、价格、赠送100→90/冷却、借用机制、v17 检查点、准备/判胜语义；投票 MSG 20/21/22 不动；血量显示与划取开关均为本机设置不进联机载荷。纯展示预计保持 Build55。
4. **验证边界**：Android 提交须保持共享头字节一致并通过 `generate-port-version.ps1 -Check`；Android 构建唯一入口 `tools/android/build-android.ps1`；MuMu 当前 ADB `127.0.0.1:16448`。每批交付按总计划 §5：截图按布局类型抽取、不动 `onlineGameReady`、存档先备份。
5. **文档债**（顺带清）：`OnlineStatusWidget.h:20-24` 头注释与 `GetSeatLine/GetStateLine` 死代码；Almanac 单株翻页无滚轮/键盘（对齐 PC 查看器时统一口径）。

## 6. 待拍板问题（开工前定案）

1. **划取阳光开关挂 Android 哪里**：PC 口径是高级选项页；Android 对应物是 RunFlowScreen 选项页，另一候选是暂停设置（NewOptionsDialog）。默认对齐 PC（RunFlowScreen 高级选项），如无异议按默认做。
2. **`2eb8646` 植物三选一跨屏冷却迁移归属**：属规则迁移非 UI；可随第四批三选一改版同批迁（改到同域文件），也可走独立同步批。默认独立同步批、不混 UI。
3. **Android 图鉴词条形态确认**：总计划已写「优先对齐 PC 介绍后附词条」——确认后 Android 底部切换钮撤还是保留为长内容入口，第三批方案稿给图再定。
4. **暂停/设置分组的版式**：PC 对话框实际 423×498，塞语言+血量+词条入口+既有项的分组方案（是否引入分页）属第三批方案稿；仅提示：PC 现有 8 控件无分组，空间余量待测。

## 7. 提交切分草案（草案待用户审，实施时逐笔确认）

**第三批（建议 5 笔，3.1 为独立提交）**
- 3.1 划取阳光 Android（F03）：触摸滑动收取+开关+持久化+捕获优先级；定点验证（连续滑过不重复入账、持卡/面板/观战不穿透、排除展示阳光）。
- 3.2 PC 本局查看器重做：读 `GetEffectiveBuffCount`、永久/借入分开、说明与翻页详情；不动协议。
- 3.3 Android 本局查看器迁入（F04）：对齐 3.2 信息结构，触摸翻页显式按钮。
- 3.4 图鉴双端口径（F07）：Android 默认附词条+分页；PC 核对。
- 3.5 暂停/设置分组双端 + Android 血量触摸开关（F06 余项）。

**第四批（建议 5 笔）**
- 4.1 主菜单双端（导航含义澄清+状态条死代码清理）。
- 4.2 模式/高级选项重排双端（PC 受高度硬约束先做信息组织；Android 换原版面板/石材视觉）。
- 4.3 三选一/投票/钱包操作区重排双端。
- 4.4 结算：Android 奖杯屏（F10）+ PC 核对；断线/失败提示一致性顺带核。
- 4.5 商店/花园一致性巡检（可并入 4.1-4.4 顺带做，不单独立批）。

每笔独立提交、增量构建、双端共享头一致；完成记录区分「源码完成/已构建/已安装/实际验收」，同批更新 `docs/08-更新日志.md`（玩家口径）。
