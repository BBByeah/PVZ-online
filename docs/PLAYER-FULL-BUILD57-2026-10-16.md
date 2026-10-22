# Build57 双端完整玩家测试包 · 2026-10-16

用户明确要求双端完整包，带资源、汉化和依赖，下载、安装/解压后即可玩，不让玩家手动补文件、导入或配置环境。已写入工作区根`AGENTS.md`、PC`CLAUDE.md`、`docs/规范.md`及Android`DEVELOPMENT.md`。此前`Test`更新包作为历史保留，玩家应使用本次`Full`包。

## 文件

工作区`dist/`：

| 产物 | 字节 | SHA256 |
|---|---:|---|
| `PvZ-Online-Build57-Full-20261016-PC.zip` | 60,808,806 | `ecc31e1426cf767bb966005e65410aa49e4f659bbc31dffc0a26825c705603f6` |
| `PvZ-Online-Build57-Full-20261016-Android.zip` | 78,784,274 | `9b7e3d851ac9917675e38726c9d0499e815917506b0d2d30f32d5f64dc1305b4` |
| `PvZ-Online-Build57-Full-20261016-Both.zip` | 139,597,040 | `c33a86ccac8dbd336310a7f55e0a517cf91f3d7bf5b0b81c4983f593e9487535` |

直接安装的APK：`dist/PvZ-Online-Build57-Full-20261016/Android/PvZ-Online-Build57-Full-20261016-arm64.apk`。完整逐文件哈希见双端合包`SHA256.txt`和`android-artifacts/full-android-build57-20261016/delivery-manifest.json`。

PC包含Release程序、main.pak、pvz_zh.pak、properties、bass.dll、9个微软x86运行库、启动脚本及玩家说明。Android同样内置两个pak和6个properties文件，附中文字体及原生运行库。没有打入个人存档、缓存、诊断文件或开发用map/pdb。

## Android首次启动

新增轻量启动页，先在后台自动释放并核对内置资源，再进入原游戏。文件按SHA256及大小校验，通过临时文件完成写入；准备标记阻止半套资源被当作完成。进程中断/空间不足时可以重试。已有完整资源直接跳过，存档目录不在资源复制范围。

默认`build-android.ps1 -Stage Apk`现在自动准备本地资源并打完整APK；只有明确使用`-WithoutGameResources`才生成开发用无资源包。资源从工作区`runtime`白名单读取，暂存到仓库外的`android-artifacts`，不提交Git。旧`package-test-build57.py package`默认转到完整打包脚本；仅显式`--updates-only`沿旧更新包路径。

Android包名仍为`io.github.pvzonline.android`、versionCode49、版本名`0.1-online57.c01ab12b-full`，签名沿用现有测试证书，可覆盖同签名旧测试版。最低Android7.0，仅arm64-v8a。玩法和协议保持Build57 / Protocol1。

## 验证

- PC Release增量构建通过；从完整ZIP解压的副本运行到中文游戏/新用户及欢迎画面，800×600客户区1:1。测试只改副本partner.xml中的产品存储名称和空注册表键，使存档写到独立`C:/ProgramData/PvZFull57Smoke20261016`；程序字节、游戏及汉化资源与交付包相同。实际加载的bass.dll、MSVCP140.dll、VCRUNTIME140.dll均来自包目录。
- Android自动准备使用生产Java类在真实资源上检查13项：全新安装、8个文件哈希、保存已有存档、已有安装跳过、准备中断标记、阻止半套资源运行、重试恢复、坏资源拒绝与路径限制，全部通过。最初检查发现Windows下renameTo无法替换中断残留，已修复并重跑通过。
- 使用独立包名`io.github.pvzonline.full57test`全新安装MuMu测试，从空数据目录自动准备资源，进入中文标题、创建测试用户、主菜单和闯关设置。设备上两个pak哈希与打包源一致。再次冷启动直接进入游戏，资源修改时间未变，没有重复释放。临时测试应用已卸载，原应用未卸载、未覆盖安装。
- 最终交付APK随后使用正式测试包名重新构建；签名v2、16KB对齐、实际launcher/ABI、所有内置资源与源文件SHA256、打包原生库与Gradle剥离库一致性检查通过。三个ZIP完整性通过。
- PC第一次抓图因隐藏测试窗口无效，后续实际显示与激活测试窗口后取得有效1:1图；另一次连续模拟点击结束了测试进程，未产生崩溃报告，不将该次当作完整游戏流程通过。第二次启动和依赖加载检查正常，仅结束了我们创建的测试进程。

这是完整可用的玩家测试包；本轮验证安装与启动，未把它当作完整混合组队整局、全部关卡和真实ARM设备兼容性已经验收。此前15项玩法定点检查仍适用于未修改的游戏代码。本次没有指定外部接收渠道，因此只生成可转发文件，没有向玩家实际发送或上传。

构建、资源清单、运行截图和日志见工作区`android-artifacts/full-android-build57-20261016/`，打包脚本`tools/package-full-build57.py`。来源为当前工作树，包括此前未提交改动，不仅是HEAD提交。
