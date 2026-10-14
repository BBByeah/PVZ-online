# PC 与 Android 同步更新

日期：2026-10-14。用户要求为持续同步更新做准备。本流程已提供来源清单、候选差异审计、版本生成与一致性检查；不自动把正在开发的 PC 工作区覆盖到 Android。

## 当前固定版本

[pc-source-baseline.json](compatibility/pc-source-baseline.json) 是 Android 已迁规则的唯一来源清单：PC `61c37d6` / Build 44 / Protocol 1。生成的 `src/Lawn/Run/PortVersion.h` 用于界面规则版本和会话编译断言。元数据检查只证明清单、头文件和协议常量一致，不替代效果消费者审查或双端运行。

`onlineGameReady=false` 目前保持不变：传输/会话底层已编译，游戏侧的开局、就绪、漏怪和团队结算尚未接入。还不能把手机与 PC 显示相同 Build 当作已可联机。

PC 当前候选为已提交 `0e08297fb047cda41b735d7be092a2bdf499ca90` / Build 45。首次审计从固定 Build 44 出发，列出 21 个变更文件，涉及规则/实体、检查点、队伍投票协议和 UI；见 [候选报告](compatibility/updates/61c37d6-to-0e08297.md)。此候选尚未迁入，不改当前 APK 的 Build 标识。

## 更新顺序

1. PC 开发者先提交一个可移植的规则版本。可以给提交打 `rules/build-45` 这样的标签；Android 仍固定完整 SHA，不追逐分支 HEAD 或未提交文件。
2. 在 Android 仓库运行只读审计，指定 PC 仓库目录和明确候选提交。报告以“旧基线→新候选”命名，避免后续更新覆盖上一轮审查。
3. 先迁实际消费者，再处理新枚举/实体字段、初始化与删除、旧档默认值、固定宽度存档/协议及中英文触屏 UI。Windows 专属代码只做对应适配。矩阵记录真实挂点，不用抽取白名单或 Build 号替代实现。
4. 只编译并定点检查受影响机制。新旧检查点、原有 v4、局内新增状态和双端参数分别核对；不每轮重新完整打通所有关卡。
5. 确认规则与协议迁完后更新来源清单，生成版本头、打包双方版本并记录 APK 哈希。若本轮只有一部分能力就绪，清单记录实际范围，继续保持联机入口关闭；不对 PC 单方面发明新协议字段。

```powershell
# 从 Android 仓库运行；只读指定 PC 提交，不写 PC 工作区。
.\tools\sync\audit-pc-update.ps1 -PcRepository ..\re-plants-vs-zombies -Candidate 0e08297

# 在实际迁移完成并更新来源清单后生成；协议常量不一致会直接报错。
.\tools\sync\generate-port-version.ps1
.\tools\sync\generate-port-version.ps1 -Check
```

APK 构建脚本会检查生成头和固定协议常量，防止来源清单已改而代码仍混用旧常量。后续 CI 可以复用同一 `-Check` 命令，避免维护第二套判断逻辑。

## 双端交付边界

联机接入时继续沿用 PC 的 START_LEVEL 全部参数、就绪屏障、席位乘数、漏怪/观战字段与团队结算。只有双方一起更新时才能扩展线格式；未经完整适配的旧版本连接策略要明确，不只看 PROTOCOL_VERSION。当前 PC 会话允许 Build 不同并提示，这项来源行为不等于 Android 已保证跨 Build 战斗兼容。

PC Build 45 的投票规则会影响多人选择、共享结果与检查点，必须把相关 RunState/NetSession/游戏侧消费一起迁入。Android 已有的独立源码、APK 与资源隔离方式保持不变；正版素材不进入共享仓库。
