# PC 更新候选审计

当前固定来源：`8bfc67ea9c85be37cac0a182c3416f3add2c1640` / Build 46。
候选已提交来源：`0a46b1a76eea944f518bbd468da651e4b571741b` / Build 51 / Protocol 1。

本工具只读取提交，输出变更与审查范围；不修改 PC 工作区，不更新 Android 来源、不复制代码、不宣称新版本兼容。

| 状态 | PC 文件 | Android 需核对 |
|---|---|---|
| M | `CMakeLists.txt` | 人工审查相关性，不自动复制无关 Windows 文件 |
| M | `Lawn/Board.cpp` | 开局/就绪屏障、团队结算、难度、生命周期 |
| M | `Lawn/Board.h` | 开局/就绪屏障、团队结算、难度、生命周期 |
| M | `Lawn/Online/NetProtocol.h` | 线格式、会话语义、双端版本/能力核对 |
| M | `Lawn/Online/QuickChat.h` | 线格式、会话语义、双端版本/能力核对 |
| M | `Lawn/Plant.cpp` | 效果消费者、实体默认值、局内存档、漏怪/观战状态 |
| M | `Lawn/Plant.h` | 效果消费者、实体默认值、局内存档、漏怪/观战状态 |
| M | `Lawn/Projectile.cpp` | 效果消费者、实体默认值、局内存档、漏怪/观战状态 |
| M | `Lawn/Projectile.h` | 效果消费者、实体默认值、局内存档、漏怪/观战状态 |
| M | `Lawn/Run/RunBuffs.cpp` | 闯关规则、确定性抽取、检查点兼容、迁移矩阵 |
| M | `Lawn/Run/RunBuffs.h` | 闯关规则、确定性抽取、检查点兼容、迁移矩阵 |
| M | `Lawn/Run/RunState.cpp` | 闯关规则、确定性抽取、检查点兼容、迁移矩阵 |
| M | `Lawn/Run/RunState.h` | 闯关规则、确定性抽取、检查点兼容、迁移矩阵 |
| M | `Lawn/SeedPacket.cpp` | 人工审查相关性，不自动复制无关 Windows 文件 |
| M | `Lawn/System/SaveGame.cpp` | 人工审查相关性，不自动复制无关 Windows 文件 |
| M | `Lawn/Widget/Almanac.cpp` | Android 交互与中英文 UI 适配 |
| M | `Lawn/Widget/NewOptionsDialog.cpp` | Android 交互与中英文 UI 适配 |
| M | `Lawn/Widget/RunPickDialog.cpp` | Android 交互与中英文 UI 适配 |
| M | `Lawn/Widget/ZombieVoteDialog.cpp` | Android 交互与中英文 UI 适配 |
| M | `Lawn/Zombie.cpp` | 效果消费者、实体默认值、局内存档、漏怪/观战状态 |
| M | `Lawn/Zombie.h` | 效果消费者、实体默认值、局内存档、漏怪/观战状态 |
| M | `LawnApp.cpp` | 开局/就绪屏障、团队结算、难度、生命周期 |
| M | `LawnApp.h` | 开局/就绪屏障、团队结算、难度、生命周期 |
| M | `SexyAppFramework/SexyAppBase.cpp` | 人工审查相关性，不自动复制无关 Windows 文件 |
| M | `SexyAppFramework/imagelib/CMakeLists.txt` | 人工审查相关性，不自动复制无关 Windows 文件 |
| M | `SexyAppFramework/widget/WidgetManager.cpp` | 人工审查相关性，不自动复制无关 Windows 文件 |
| M | `SexyAppFramework/widget/WidgetManager.h` | 人工审查相关性，不自动复制无关 Windows 文件 |
| M | `docs/03-过程与问题.md` | 人工审查相关性，不自动复制无关 Windows 文件 |
| M | `docs/04-联机-M2.md` | 人工审查相关性，不自动复制无关 Windows 文件 |
| M | `docs/06-成长-植物buff方案.md` | 人工审查相关性，不自动复制无关 Windows 文件 |
| M | `docs/07-强度与难度调整日志.md` | 人工审查相关性，不自动复制无关 Windows 文件 |
| M | `docs/08-更新日志.md` | 人工审查相关性，不自动复制无关 Windows 文件 |
| A | `docs/10-植物buff扩充定稿.md` | 人工审查相关性，不自动复制无关 Windows 文件 |
| A | `docs/PC-快捷键.md` | 人工审查相关性，不自动复制无关 Windows 文件 |
| A | `tools/mk_run_checkpoint.pl` | 人工审查相关性，不自动复制无关 Windows 文件 |

迁入消费者、实体默认值、存档/协议和 UI 依赖后，做受影响机制的代表检查，再推进 pc-source-baseline.json、生成 PortVersion.h 和双方交付记录。新关、新档与旧档恢复分开核对；不通过只改 Build 号来假装同步。
