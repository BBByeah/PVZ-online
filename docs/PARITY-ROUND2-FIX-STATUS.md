# 第二轮审计修复交接

2026-10-16。双端同步修复F13–F18，附带相关F01/F02退出与选择存档。主交接在同级Android仓库`docs/PARITY-ROUND2-FIX-STATUS.md`，审计与证据也在那里。

## F13/F14：完整选择阶段与钱包事务

检查点升v17，保留待选次数、候选、盐/计数、本屏重摇价格、追赶目标和植物/buff候选去重集。v2–v16兼容链保留。付费/券重摇通过单份持久事务日志提交完整档案和检查点，写日志失败回滚扣款和候选；日志已提交而文件发布中断则在下次读写时幂等完成。普通存档也使用临时文件原子替换。

新局及选择阶段及时保存；Android奖励退出正常发退关。PC Release、Android Native通过，隔离档案的8项往返/扣款/中断恢复/写失败/旧档检查全部通过，进程退出0。证据目录`../android-artifacts/parity-audit-round2/`，未修改真实用户档案或替换正在运行的程序。

## F15–F18收口（Build55）

- 无尽保留植物根据当前生命规则刷新最大生命，保留受伤比例；后续叠层生效、同规则不重复乘算。
- 主动赠送切入100扣款/90到账的回执、重试和去重链；已离线目标不扣款，负回执只退款一次，未知送达不盲目退钱。手机/G入口均要求55。
- 取消只释放匹配且未确认的借出名额并保存；取消请求跨短暂断链重试，已确认借用和不同请求受保护。
- 新关/同关重试产生新readyEpoch；START_LEVEL=41字节、SEEDS_READY=7字节。旧就绪不放行，下一轮早到报告保留，同轮定向重发不清就绪。最多六席仍逐席判断，不写死两个玩家。
- 保留并同步本轮中途PC54的难度规则到Android，修正收到首领档2却被清零的守卫。当前需双端55，中继无需修改；没有另改费用或成长数值。

PC真实棋盘/两生产会话加一个原始席位的18项检查全部通过；最终8项存档检查以当前结构重编后仍全部通过。双端12项关键函数/协议归一化相同。最终PC Release、Android Native通过；手机收尾时启动原MuMu实例并覆盖安装新APK，仍未做手机玩法运行验收。全部证据在上面的parity-audit-round2目录，无新截图或玩家存档写入；临时中继已清理。

APK/PC产物及安装结果如下。第一轮审计的其他独立功能缺口仍保留，onlineGameReady=false。

## 最终产物与安装

- PC代码来源`b690bc64ca3833c86bfe874275a95d419969c273`；Android代码来源`4df6a3ddc2a8ef456aa7e5ea10ac7d9dfc6cf12a`（以下是这份包的来源，不因后续仅文档提交改变）。
- PC候选`../android-artifacts/pvz-online-build55-pc.exe`，4,964,864字节，SHA256 `CA3AAA760EE8460D5B9E38FBBE89814E9C41C86EEBC17ED5A97CB92EF33465D5`。确认PC游戏未运行后已更新`../runtime/SexyAppFramework.exe`，原exe备份在证据目录`pc-before-build55.exe`，没有结束玩家对局。
- APK `../android-artifacts/pvz-online-build55-parity-fixes-arm64.apk`，28,385,508字节，SHA256 `21874BBC0D6F1C8372F7D7DE4274A6D4C1F56276D9B3DBDCA3287B900A455FDF`。versionCode37，versionName `0.1-online55.4df6a3dd`。
- Gradle成功，APK v2签名及16KB zipalign检查通过；包内libmain.so与本次Gradle剥离符号输出哈希一致。JNI输入和包内剥离后库哈希不同属于剥离步骤，不能因此误判装入旧库，也不凭固定来源常量未被使用时在二进制中找不到字符串就判包错误。
- MuMu官方管理器确认实例2/Android15已关闭后，本轮启动原实例；动态ADB端口为`127.0.0.1:16448`，不是旧7555。覆盖安装成功，设备实际base.apk SHA256与上述APK完全一致，包管理器确认37/online55。
- 安装前后5个原用户文件逐一SHA256相同，备份`device-before/`、安装后副本`device-after/`和核验`fix-device-data-check.json`在证据目录。没有清数据或选择游戏关卡；本轮没有启动新包做手机玩法实测，模拟器保留运行供用户使用。
- 汇总清单`fix-artifacts.json`、安装`fix-device-install.log`、版本`fix-device-version.log`、设备包哈希`fix-device-base-sha256.log`均在证据目录。未推送GitHub或公开发布。
