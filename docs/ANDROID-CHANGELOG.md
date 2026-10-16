# Android 开发更新记录

## 2026-10-16 · Build55 UI第二批：协作直接操作

- 同PC重排消息、表情、阳光为三个常驻入口；点入口再点目标即可完成发送、赠送或求援。余额不足仍可打开求援，等待回执时防重复赠送；保持扣100到账90、原冷却和协议。
- 借用同页选择队友与植物，显式发送；“收起”保留申请，“取消申请”等待确认。原版石材与种子卡，词条实际层数及长详情分页。
- 高频面板外仍可种植，拖出松手、关闭、失焦迟到松手与席位换人时不误操作；通知最多两行。双端共享布局与点击逻辑，Native编译通过，设备交付结果见[第二批交接](UI55-BATCH2-STATUS.md)。

## 2026-10-16 · 通用词条三处数值回调

- 同 PC `5c0d110`：火力（子弹伤害）+20%→+25%；急袭（攻速）每层 ×0.75→×0.8；速种（种植冷却）每层 ×0.8→×0.85。仅全局表三行 + 枚举注释，单株词条与协议/存档不动。Native/Apk 编译通过（APK 未安装设备）。

## 2026-10-16 · 三选一植物屏跨屏冷却同步

- 移植 PC `2eb8646`：跳过或重摇刷掉的植物，下一屏不再出现；选走的植物入池后由 owned 过滤天然挡住。与增益屏同口径，纯内存记账、不落检查点。
- Run/RunState.cpp/.h 手工重放（Android 植物分支旧式重抽循环换成 PC 的候选摘除集结构）；Native/Apk 编译通过（APK 未安装设备，沿用「接力批统一重打再装」口径）。

## 2026-10-16 · 常用键H/J血量与VK路由纠错

- 主要操作为键盘上排1–9/0选卡、Q铲子、H僵尸血量/J植物血量；血量不使用Fn键。小键盘仅为可选兼容输入。
- 修复把VK键码当小写字符导致的协作误触；H/J长按不反复切换，不再同时触发调试刷怪。补己方本体/防具与植物血量数字绘制，触摸设置入口尚未补。
- 固定PC实现来源`1d341ab`；双端编译、真实原生键路由及绘制定点通过，移动设备运行未重验。PC37–53主线、当前值与历史提交见PC `docs/11-Build37以来变更总览.md`；接续见[本轮记录](KEYBOARD-HEALTH-FIX-STATUS.md)。

## 2026-10-16 · 全局火力削弱同步

- 全局词条「火力」每层 +30%→+20%（∞ 不变）；其余全局条维持现值、不加层上限——同 PC 2026-10-15 定案（PC `e69c6cf`，用户拍板否掉层上限方案）。
- 仅改 Run/RunBuffs.cpp/.h 表行与注释；改后与 PC HEAD 逐字核对一致。不含接力标记批的改动（那批另行提交）。
- Native/Apk 编译通过；APK 未安装设备——待接力标记批定稿统一重打后再装，避免中途换包。

## 2026-10-16 · Build53 接力怪标记修复

- 将队友传来的僵尸金色箭头移到棋盘最终叠加层，避免被模型/防具/重叠单位遮住；仍只标接收怪及其子单位。
- 新增可选v4存档块106保存标记ID，兼容无此块的旧存档；旧档已经丢失的来源无法还原。PC使用对应的可选尾块。
- 双端编译通过，PC真实渲染及保存/旧档定点通过；Android运行未重验，新包未覆盖安装。交付产物与数据边界见 [标记复查交接](RELAY-MARKER-FIX-STATUS.md)。

## 2026-10-15 · Build52 借用与阳光求援

- 双端关间申请借植物，对方确认后专属词条逐项取最高、同种卡和植物共享；下关恢复，变身不撤回，永久奖励池独立，v16保留同关借用并兼容旧档。
- 战斗第9条短语/协作页求援，10秒有效、15秒请求间隔；响应扣100到账90，回执和去重防止重发复制阳光，结果未知不自动退款。
- 移动第五按钮“协作”，固定PC d30f30a / Build52；无需修改Go中继，双方均须更新。
- 双端编译与21项定点通过，Android对原生NetSession验证借入及阳光50→140、重复仍140；完整双端实战与观战未验。五个原用户文件已恢复并核对哈希。最终产物和接续见[持续交接](COOP-ROLLOUT-STATUS.md)。

## 2026-10-15 · Build 51 开发候选与协作接续

- 固定同步至 PC `c79c1b3`（含 `2da2cd0` 坚果保龄球） / Build51，迁入新增植物强化、重摇券、护心首伤与九条短语/表情新编号；检查点 v15，新实体状态使用独立可选块。
- 接入按需观战、切换队友/返回与触摸输入隔离，补推车/护具/截断提示；真实双端观战仍待检查。
- 双端接力怪箭头首批、移动端石材按钮/九块面板与图鉴词条循环页已编译；只有主菜单、房间做了定点画面检查。
- Native/Gradle、v2 签名和 16KB 对齐通过；候选已安装 MuMu，五个用户文件哈希一致。完整玩法未验，onlineGameReady=false 保留。
- 阳光求援、借用与换位优化尚未实现；当前不是最终交付。产物哈希、所有未验项与继续步骤见 [持续交接](COOP-ROLLOUT-STATUS.md)。

## 2026-10-14 · 手机协作与 Build 46

- 记录非原版风格 UI 清单与替换顺序；整体美术重做尚未执行。
- 三个石材触屏按钮实现快捷消息、植物表情与送阳光；沿 PC 的 ID、扣款/到账/冷却，接收横幅和阳光展示不重复入账。
- 固定 Build 46 同步血量饱和计算、首领血量上限接力、16 位投票与原生投票框、重摇币面显示修正。
- MuMu 与固定 Windows 会话验证端完成代表检查；完整 PC 战斗与跨关仍待验收。APK 与保存边界见 [执行记录](MOBILE-COOP-STATUS.md)。

### English

Added touch chat, plant emotes and targeted sun gifts using PC rules and native stone art. Pinned Build 46 ports saturated health, relayed boss identity/max HP, 16-bit votes with a native dialog, and coin display fixes. A UI style audit records the later reskin work. Representative native-peer checks passed; full PC gameplay and team transitions remain pending.

## 2026-10-14 · 中继连接流程修复

- 核对固定 PC 面板，分开「主机 IP」与「服务器」，加入房间自动使用中继端口 97，直连仍使用 27777，移除混用的端口输入框。
- 默认服务器、创建/加入房间与输入框回车流程沿用 PC；连接状态明确显示方式与实际端口。
- Android 已实际加入用户的 PC 中继房间并收到双席位名册；保持协议和玩法规则不变。最终包与验证边界见 [联机记录](NETWORK-GAME-STATUS.md)。

### English

Matched the pinned PC lobby flow: separate direct host IP and relay server fields, with the session choosing port 27777 or 97 automatically. Room creation/join and Enter shortcuts follow PC. Connection status now includes the transport and port. Android successfully joined the user's real PC relay room; battle and team-transition validation remain pending.

## 2026-10-14 · 房间与游戏侧联机

- 主菜单加入中英联机入口、直连/中继房间、输入框和席位名册；接入 Android 网络权限与会话更新循环。
- 沿固定 PC Build 45 接入 START_ACK/RUN_GO、奖励/补发、大小卡池选卡就绪门、漏怪接力、席位阳光/出怪、末席推车、暂停与团队胜负/重试。
- 联机棋盘不覆盖冒险局内档；退关清理失败弹窗，客户端不能独自重试。修复闯关两株卡池触发戴夫强选三株的崩溃。
- 最终 APK 与固定 Windows 原生会话端双向开局检查通过；完整 PC 游戏、中继与整队跨关仍待代表验收。已恢复五个原用户文件，哈希一致。交付与限制见 [NETWORK-GAME-STATUS.md](NETWORK-GAME-STATUS.md)。

### English

Added bilingual direct/relay lobby UI and Android networking permission. Pinned Build 45 game hooks now cover start acknowledgements, rewards, seed-ready barriers, zombie relay, seat rules, pause and team outcomes. Online boards preserve adventure saves; exit clears failure dialogs and clients cannot restart the team alone. Fixed Crazy Dave picking three plants from a two-plant run pool. The real APK passed bidirectional checks with a pinned Windows native session peer; full PC gameplay, relay and team transitions remain to be validated. Original user files were restored with matching hashes.

## 2026-10-14 · Build 45 规则与存档同步

- 固定 PC `0e08297`，迁入五档场景阶梯、八条僵尸强化消费者、花盆/荷叶抽取权重和首次付费重摇。
- 闯关检查点升至 v14，旧 v13 默认行为保留；Portable v4 可选块 104 保存免控字段。
- 同步 36 字节开局快照及三候选/弃权投票消息。固定 PC 尚无开票/计票游戏调用，Android 暂不开放投票与联机入口。
- 有限 MuMu、ARM64 状态/消息和旧档检查通过；五个用户文件恢复且 SHA256 一致。交付及未验范围见 [BUILD45-SYNC-STATUS.md](BUILD45-SYNC-STATUS.md)。

### English

Pinned Build 45 scene ladders, zombie-buff consumers, weighted plant picks, paid-first rerolls and v14 saves are ported. Optional v4 chunk 104 preserves immunity. START_LEVEL snapshots and three-choice vote messages compile; game voting and multiplayer remain pending. Limited checks passed and original user files were restored. See the linked status for coverage and APK hash.

## 2026-10-14 · 防具变体、高级单机选项与同步更新准备

- 迁入 PC 八种防具变体及完整出怪名单，定义、报纸帽动画、防具/攻击/死亡、缓存边界与固定 PC 一起适配。
- 加入高级难度/规模/节奏、植物僵尸、无尽场景和关底首领规则；无尽过关保留草坪与阳光。旧 v4 波表布局不变，新容量与状态用可选扩展块。
- MuMu 代表类型、中文入口、无尽 48 株选卡及旧档读写通过，五个用户文件已恢复且哈希一致。首领和无尽跨关未完整实测。
- 网络传输/会话底层已编译，游戏侧联机仍未接通。新增固定来源 JSON、候选差异审计、版本生成与 Native/Apk 构建一致性检查；已列出 Build 45 候选，当前仍固定 Build 44。
- 最终 APK 与真实验收范围见 [VARIANTS-OPTIONS-STATUS.md](VARIANTS-OPTIONS-STATUS.md)，持续更新见 [PC-SYNC-WORKFLOW.md](PC-SYNC-WORKFLOW.md)。

### English

Eight PC armored zombie variants, their definitions/animations/cache bounds and the full pinned roster are ported. Advanced solo options, fixed-scene endless runs with lawn retention, and final-boss rules are integrated. Optional v4 chunks preserve the original wave-table layout. Representative MuMu checks and old-save read/write checks passed; the five original user files were restored with matching hashes. Transport and session code compile, but game-side multiplayer remains pending. A pinned source manifest, read-only PC update audit, generated version header and build checks prepare ongoing synchronization. Build 45 is audited as a candidate; the delivered rules remain Build 44.


## 2026-10-14 · 完整 buff、倍选与金币重摇

- 固定 PC 61c37d6 / Build 44，全量迁入 96 条目录的实际消费者；92 条可抽取、3 条存量、1 条沿 PC 停用，常规植物池开放 48 株。
- 接入多发/攻速、穿透、范围/直伤/护甲、持续效果、连砸/补雷/吞噬/再生等状态及植物依赖；费用、冷却、Kind 与叠层取整沿用 PC。
- 加入 ×1/×2/×3 增益选择倍率、每屏免费刷新与金币阶梯重摇，闯关金币正常记入冒险档案；选择页补完整中英说明。
- Portable v4 增加可选效果扩展块，旧 Android 冒险局内档可恢复；MuMu 代表机制、实际扣费保存和新字段读写通过。测试结束恢复五个原始用户文件，哈希一致。
- 最终 ARM64 APK 已构建并更新安装；完整验收范围和哈希见 [BUFF-PORT-STATUS.md](BUFF-PORT-STATUS.md)。联机、新增僵尸与高级模式仍待迁入。

### English

The Android run now ports all active buff consumers from pinned PC commit 61c37d6 (Build 44): 92 selectable effects, three retained legacy effects and one disabled entry. All 48 ordinary plants, x1/x2/x3 buff picks and escalating coin rerolls are supported. Run coins enter the adventure profile as on PC. An optional v4 chunk stores new effect fields while existing Android adventure saves remain readable. Representative MuMu checks, real wallet persistence and effect-state round trips passed; the original five user files were restored with matching hashes. Networking, new zombie types and advanced modes remain pending.


## 2026-10-13 · 五场景战斗基础与重打

- 闯关出怪表、权重、波数、标准场景难度及血量接入固定 PC 来源；未实现僵尸 34–41 过滤，每波上限 20 只。
- 基础植物抽取池增至 39，蘑菇白天不睡，水生植物可在非屋顶陆地种植；卡池不超过 10 时自动带入。
- 单机不生成推车与购买的耙子，不享受急救购买加成；失败保存计数，重打保持关号、卡池、增益和种子。
- 阻止随机花园植物/巧克力进入原版档案。MuMu 代表性场景种植和白天真实胜利通过，检查后恢复用户检查点，原版存档哈希不变。
- 仍只开放火力、扎根、速种。联机、新增僵尸、特殊植物与完整增益继续开发；本批不代表全关卡平衡验收。

### English

Finite solo runs now use the supported PC zombie roster, wave counts, scene budgets and health scaling. The base plant pool grows to 39, mushrooms stay awake, and aquatic plants can be planted on non-roof land. Retries preserve the checkpoint and seed; solo runs have no mowers or purchased advantages. Representative MuMu checks passed with the original profile unchanged. Networking, new zombie types and the remaining buffs are still pending.

## 2026-10-13 · 底座与拖放

- 首个 ARM64 开发 APK 可在 MuMu Android 15 安装启动，资源由玩家自己的正版副本提供。
- Android 从种子卡拖到有效格子后松手种植，无效落点取消拖动；继续支持点卡片再点格子。
- APK 名称 PvZ Online Dev，包名 io.github.pvzonline.android，与社区原版分开。

## 2026-10-13 · 单机闯关原型与语言

- 新增闯关入口、25/10/5 关模式、开局植物/增益选择、换关奖励、刷新/跳过和独立检查点续玩。
- 当前只提供火力、扎根、速种三种已接入效果；其他 PC 条目未进入抽取池。出怪、难度、推车与全部植物特效还需继续迁移。
- 新增自动/中文/English 语言选项，保存后重启生效。原版菜单、字体、图鉴和提示复用玩家本地的 PC pvz_zh.pak；新增界面与资源管理页提供中英双语，内置开源中文字体补缺字。
- 成就文本接入 PC 原有翻译键。闯关内可查看原版图鉴。

当前尚未包含合作联机。此记录不代表 Windows MOD_BUILD 发生变化。
