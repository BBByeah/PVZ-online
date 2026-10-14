# Android 开发更新记录

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
