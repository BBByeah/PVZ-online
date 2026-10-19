# Windows 参考快照

来源仓库：`https://github.com/Not-a-Different-Name/PVZ-online`。
提交：`7a756a2db2ef88b10a08ec4d5c34900818a4995c`。

ConstEnums.h、NetProtocol.h、RunZombieRoster.h/.cpp 通过 git show 从上述本地提交导出，不含工作区未提交改动；它们只作为编号与出怪表参考，不编入 Android。实际编入 src/Lawn/Run 的 RunState 与 RunBuffs 同样来自此提交，已适配 Portable API，并限制抽取池为当前已实现的效果与植物；来源 CC0 许可见本目录。

保留来源仓库的 CC0 许可文本 `LICENSE.CC0-1.0`。此目录的许可说明只对应这组来源文件，不替代 Portable 主工程的许可。

2026-10-13 批次 A：实际 src/Lawn/Run/RunZombieRoster.h/.cpp 已从此快照适配并编入 Android，保留现有编号、过滤未实现的 34–41；本目录快照继续只作参考。标准档消费出怪预算、场景血量和种植基础规则，具体差异及有限验证见 docs/NEXT-BATCH.md。

2026-10-14 完整 buff 计划的 PC 来源更新为已提交 `61c37d625ccb3cb7cecaed2422798e8ec5746cbf`（MOD_BUILD 44）；本目录仍是原 7a756a2 参考快照，没有伪装为新版本。两提交之间 buff 表与 Plant/Projectile/Zombie 效果代码未变，新增倍选、金币重摇与对应协议变化需从新固定提交读取。完整范围见 docs/BATCH-B-PLAN.md 和 docs/compatibility/buff-port-matrix.csv。
