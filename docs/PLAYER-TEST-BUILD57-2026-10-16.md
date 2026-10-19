# Build57 双端玩家测试包 · 2026-10-16

> 本文记录的不带资源更新包已被用户要求的[双端完整测试包](PLAYER-FULL-BUILD57-2026-10-16.md)替代；给玩家请发名称含`Full`的新产物。

按用户要求打出可转发的测试包，包含当前双端UI、美术恢复、紫卡能力和平衡调整，以及[发布前校准](RELEASE-CALIBRATION-2026-10-16.md)中的Android四处修复。没有增加玩法改动，双端仍为MOD_BUILD57 / Protocol1。

## 交付

工作区 `dist/` 下：

- `PvZ-Online-Build57-Test-20261016-PC.zip`：2,514,443字节；Release程序、9个微软x86运行库、启动脚本、玩家说明、许可。
- `PvZ-Online-Build57-Test-20261016-Android.zip`：21,067,267字节；APK、玩家说明、许可。
- `PvZ-Online-Build57-Test-20261016-Both.zip`：23,584,757字节；以上双端内容及总说明、逐文件SHA256。
- 直接安装的APK：`dist/PvZ-Online-Build57-Test-20261016/Android/PvZ-Online-Build57-Test-20261016-arm64.apk`。

双端合包SHA256：`fc87e807f5a43414250da18def99328f24632e3e97cb2e60c5615f0c9dc9782f`。

## 验证与范围

PC Release、Android Native、APK Release构建均通过，生产源码构建前后哈希无变化。PC导入表无调试运行库。APK v2签名、16KB对齐、ZIP完整性检查通过，打包libmain.so与Gradle实际剥离产物一致。压缩包逐个解压校验通过，未混入main.pak、汉化pak、原版素材、个人存档、map、pdb或诊断文件。

程序哈希与刚完成校准的候选相同，沿用该候选已通过的15项定点机制检查；本次没有重复宣称完成实机整局验证。测试说明明确请玩家覆盖更新本次包，备份存档并优先测试PC/手机混合组队、准备与跨关同步、漏怪接力和结算、协作及存档继续。

Android最低API24，仅arm64-v8a；versionCode49 / `0.1-online57.c01ab12b`，沿用现有Android Debug测试签名。与此前UI包版本名相同，以本次文件及SHA256区分。旧版PC局中快照不兼容，关间检查点和档案保留。

本次生成本地可转发文件，没有指定外部收件人或渠道，因此未向玩家发送，也未安装、卸载或替换本机运行中的游戏。

完整哈希、源码清单和构建验证日志保存在 `android-artifacts/test-build57-20261016/`；打包脚本为 `tools/package-test-build57.py`。源码来自当前工作树（含既有未提交改动），不是只来自HEAD提交。
