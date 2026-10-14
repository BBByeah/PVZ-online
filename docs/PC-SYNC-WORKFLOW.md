# PC 与 Android 同步更新

日期：2026-10-14。用户要求为持续同步更新做准备。本流程已提供来源清单、候选差异审计、版本生成与一致性检查；不自动把正在开发的 PC 工作区覆盖到 Android。

## 当前固定版本

[pc-source-baseline.json](compatibility/pc-source-baseline.json) 是 Android 已迁规则的唯一来源清单：PC `8bfc67e` / Build 46 / Protocol 1。生成的 `src/Lawn/Run/PortVersion.h` 用于界面规则版本和会话编译断言。元数据检查只证明清单、头文件和协议常量一致，不替代效果消费者审查或双端运行。

`onlineGameReady=false` 目前保持不变：房间和游戏侧开局、就绪、漏怪与团队结算已接入，开发入口可用于验收。真实 Android 与固定 PC 原生会话端双向检查已通过，但完整 PC 游戏客户端仍待验收；见 [联机结果](NETWORK-GAME-STATUS.md)。还不能把手机与 PC 显示相同 Build 当作所有对局路径兼容。

Build 44→45 的首次审计列出 21 个变更文件，涉及规则/实体、检查点、队伍投票协议和 UI；见 [历史候选报告](compatibility/updates/61c37d6-to-0e08297.md)。已提交依赖现已迁入；Build 46 已补齐投票调用，并增加首领接力/血量上限线格式；本轮已迁入，见 [手机协作与同步](MOBILE-COOP-STATUS.md) 及 [45→46 审计](compatibility/updates/0e08297-to-8bfc67e.md)。

## 更新顺序

1. PC 开发者先提交一个可移植的规则版本。可以给提交打 `rules/build-45` 这样的标签；Android 仍固定完整 SHA，不追逐分支 HEAD 或未提交文件。
2. 在 Android 仓库运行只读审计，指定 PC 仓库目录和明确候选提交。报告以“旧基线→新候选”命名，避免后续更新覆盖上一轮审查。
3. 先迁实际消费者，再处理新枚举/实体字段、初始化与删除、旧档默认值、固定宽度存档/协议及中英文触屏 UI。Windows 专属代码只做对应适配。矩阵记录真实挂点，不用抽取白名单或 Build 号替代实现。
4. 只编译并定点检查受影响机制。新旧检查点、原有 v4、局内新增状态和双端参数分别核对；不每轮重新完整打通所有关卡。
5. 确认规则与协议迁完后更新来源清单，生成版本头、打包双方版本并记录 APK 哈希。清单分别记录实现范围与完整验收状态；若仅开放开发验收入口，明确验证边界，不宣称全部双端玩法可用。不对 PC 单方面发明新协议字段。

```powershell
# 从 Android 仓库运行；只读指定 PC 提交，不写 PC 工作区。
.\tools\sync\audit-pc-update.ps1 -PcRepository ..\re-plants-vs-zombies -Candidate 8bfc67ea9c85be37cac0a182c3416f3add2c1640

# 在实际迁移完成并更新来源清单后生成；协议常量不一致会直接报错。
.\tools\sync\generate-port-version.ps1
.\tools\sync\generate-port-version.ps1 -Check
```

APK 构建脚本会检查生成头和固定协议常量，防止来源清单已改而代码仍混用旧常量。后续 CI 可以复用同一 `-Check` 命令，避免维护第二套判断逻辑。

## 双端交付边界

联机接入时继续沿用 PC 的 START_LEVEL 全部参数、就绪屏障、席位乘数、漏怪/观战字段与团队结算。只有双方一起更新时才能扩展线格式；未经完整适配的旧版本连接策略要明确，不只看 PROTOCOL_VERSION。当前 PC 会话允许 Build 不同并提示，这项来源行为不等于 Android 已保证跨 Build 战斗兼容。

当前固定来源已推进至 Build 46 / `8bfc67e`，规则、实体、RunState、协议和投票游戏流程已迁入。手机协作入口使用相同的快捷聊天/送阳光消息；不同 Build 能入房不代表不同线格式可混用，完整双端玩法应使用双方同一固定 Build 46。实际源码迁入与有限验证见 [本轮结果](MOBILE-COOP-STATUS.md)。Android 独立源码、APK 与资源隔离方式保留，正版素材不进入共享仓库。
