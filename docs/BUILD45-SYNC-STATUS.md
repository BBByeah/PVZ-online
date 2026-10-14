# Build 45 同步结果

2026-10-14。PC 来源固定 `0e08297fb047cda41b735d7be092a2bdf499ca90`，MOD_BUILD 45、PROTOCOL_VERSION 1。后续执行顺序见 [批次 D 计划](BATCH-D-PLAN.md)。

## 本轮完成

- 场景阶梯五档（×1、×1.2、×1.33、×1.5、×2）使用 PC 固定千分比表，旧档默认仍 ×1.33；无尽保留原曲线，不叠场景阶梯。入口中英文同步。
- 八条僵尸强化：迅捷、铁壁、护心、狂食、再生、免控、墓碑者、无视阻挡，叠层上限/数值饱和和实体消费沿用固定 PC。免控初始化及绕开谓词的直接控制写入点同时适配。
- 花盆/荷叶候选权重 ×5；免费刷新取消，首屏首次重摇也扣 100 金币，同屏后续 ×10，下一屏重置。保留档案钱包规则。
- 闯关检查点升为 v14，保存阶梯/强化档、八条层数及投票关号；旧 v13 缺省新字段。Portable v4 增加可选块 104 保存免控；旧块不改、缺块默认为 false。
- 同步 PC START_LEVEL 的 36 字节载荷和强化快照，迁入三候选投票/弃权/结果消息及会话队列。当前只编译，未接游戏侧联网。
- 沿用 PC 辣椒僵尸爆炸后摘头的修复，避免火焰计时反复重置。

## 投票与联机边界

固定 PC 提交有强化效果、存档和消息，但游戏代码尚未调用 SendZombieVote* / AddZombieBuffStack / 投票关号记账。因此本轮完成这些已提交依赖，Android 不另造投票规则、不开放无效强化开关。onlineGameReady 仍为 false；Build 45 标识表示源码规则来源，不表示已经双端互通。

下一步接房间、START_ACK / RUN_GO / 选卡就绪屏障、补发追赶及游戏循环。僵尸开票/计票跟随 PC 后续固定提交，或先确定两端共用方案。开放强化前还需一起检查高阶梯与多层铁壁的血量乘法：PC 当前使用 int 乘算，如 48000×100000 的中间值超过有符号 32 位范围，不能只在 Android 单端改口径。

## 有限检查

- 增量 Native/Apk、版本生成检查、开发 v2 签名和 16 KB zipalign 通过，已更新安装 MuMu Android 15。
- MuMu：中文模式页与阶梯切换；原用户 v13 闯关档续玩；首屏重摇标价 100、零金币不能刷新；完成六屏选择后真实 APK 写出 v14 且所选 ×1.2 档保留。
- 一次仓库外 ARM64 检查使用实际 RunState 的读写/倍率函数和真实协议头，文件接口换成内存适配器：原 v13 读取、v14 八层数/投票关号往返、五档末场景倍率、数值饱和及重摇底价通过；36 字节开局消息、三候选/弃权/结果编解码通过。这不代替 TCP 或游戏开票验收。
- 原用户冒险 v4 成功载入并重新保存；CRC 正确，新增块 104 中旧僵尸免控均为 false。检查结束恢复全部五个原用户文件，SHA256 与本轮备份一致。
- 八条强化未逐条实战验收，投票、双端网络、首领与无尽跨关仍有运行验证工作；本轮未重复完整 buff 或所有关卡测试。

APK（仓库外）：`android-artifacts/pvz-online-build45-arm64.apk`，28,069,036 字节。

SHA256：`FC4883702735B590167FC48BF11BFFAEC940C5A5411EC6C80AA0E8C7A3CDF609`。

## English

Pinned PC Build 45 rules, five scene ladders, all eight zombie-buff consumers, weighted pot/lily picks, paid-first rerolls, v14 checkpoints and optional v4 immunity storage are ported. The exact PC wire header and updated session queues compile. The pinned PC commit does not yet connect voting to the game flow; voting and multiplayer remain unavailable. Limited MuMu, ARM64 state/codec and old-save checks passed, and all five original user files were restored with matching hashes. Full buff combat and cross-platform network validation remain pending.
