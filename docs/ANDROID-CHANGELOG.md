# Android 开发更新记录

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
