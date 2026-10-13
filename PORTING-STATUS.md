# Android 移植进度

## 基线

- Portable 上游：wszqkzqk/PvZ-Portable，提交 `141c1476dd462ac3526f796e303177616f4de6bc`。
- 固定源码包 SHA256：`A3966F829E4F05FE7A1D67EBC7ECD575F84EF9DCD8E9A8418E171E7A0DD12AA4`。
- Windows 来源候选：`7a756a2db2ef88b10a08ec4d5c34900818a4995c`；现有未提交汉化代码未迁入。
- 此工程来自固定提交压缩包，没有上游 Git 历史；本地初始化提交 `ac3e1e5` 是开发基线，不能误称为上游提交。共享仓库为 `https://github.com/Not-a-Different-Name/PVZ-online-android`，整合分支 `android-port`，保留基线和拖放修复历史。

## 工具链（2026-10-13）

- JDK：CLion 自带 JetBrains Runtime 21.0.8。
- Android SDK：项目父目录 `android-sdk/`，platform 36、build tools 36.0.0、platform-tools。
- NDK：28.2.13676358；native API 24，ABI arm64-v8a。
- CMake/Ninja：CLion 自带；SDK 也已安装 CMake 3.22.1。
- Gradle：8.12，项目父目录 `android-build-tools/`。
- SDL2：2.32.10，已生成并安装 ARM64 `libSDL2.so`。
- vcpkg ports：上游锁定提交 `e8709ee8703328fe121eab33cda73a406a7626b7`，本地压缩包展开；Git registry fetch 遇到连接重置，改用此固定版本的本地 ports。

## 当前状态

### 2026-10-14 计划更新（尚未实现）

用户要求完整搬完 PC buff，并尽量保持设计/数值以便双端互通。后续统一覆盖 96 条目录、92 条源版可抽取效果及必要的 48 株植物行为；来源更新到已提交 PC 61c37d6 / MOD_BUILD 44，纳入倍选和金币重摇。金币按用户确认正常记入冒险档案。完整计划与逐条矩阵见 docs/BATCH-B-PLAN.md、docs/compatibility/buff-port-matrix.csv。下列批次 A 是当前已实现行为，现有 APK 尚未因本计划改变。

### 最新批次：五场景战斗基础与重打

- 批次 A 已实施：固定 PC 来源的现有类型出怪表、波数、抽怪权重、预算与本体/防具场景血量；巨人价值/数量限制与每波 20 只上限。PC 新增僵尸 34–41 仍过滤，保留原编号；三种时长模式按实际引擎关号查同一张表。
- 基础植物池从 28 扩到 39；蘑菇白天清醒、非屋顶陆地可种水生植物。卡池最多 10 时全部自动带入，超过 10 时选卡。咖啡豆、升级植物和其特殊规则仍未迁入。
- 单机无推车/耙子/急救购买加成；失败独立记账，重打保持关号、卡池、增益与种子。花园掉落不进入原版档案。未实现的高级选项检查点会明确拒绝。
- 定点验证覆盖一次白天真实 10 波胜利→奖励页、失败重打、夜晚向日葵、泳池睡莲+清醒小喷菇、屋顶新花盆+清醒小喷菇，以及迷雾载入。胜利使用增强现有增益的仓库外检查点，不代表标准档平衡或完整 25 关验收。原版 user1.dat、users.dat、game1_0.v4 和语言设置哈希保持一致；用户原检查点已恢复。
- 当前 APK：仓库外 `android-artifacts/pvz-online-run-rules-arm64.apk`，27,920,560 字节，SHA256 `1B23C09B682CC5C1214B042F37191F50C959C0ED81685D52485BEE2470E32A67`，增量 Native/Apk 构建、v2 签名、16 KB zipalign 通过，已安装 MuMu Android 15。下一批按完整 buff 迁移计划实施。
- 未完成：全部增益和特殊植物规则、新增僵尸、联机、原版风格 mod UI、完整五场景平衡、SAF 用户操作及真实 ARM 手机验收。

### 上一批次：单机闯关与语言

- 独立推进；同伴任务 A 的枚举报告已完成，见 docs/compatibility/enum-audit.md。SeedType 实际编号一致；PC 僵尸 34–41 尚需迁入行为与缓存。RunZombieRoster 只作为参考，不编译。
- RunState/RunBuffs 来自固定 Windows 提交 7a756a2，适配 Portable API。最小闯关入口、25/10/5 关、开局四植物/两增益、换关奖励与独立 android_run*.dat 检查点已接入。
- 只开放火力/扎根/速种，实际挂到弹体伤害、植物血量和种子冷却。植物抽取池也限制为当前已实现的 28 种；未迁入的增益不会作为空效果出现在选择中。
- MuMu 已进入闯关并种植；用临时受控结算覆盖两次奖励→下一关，检查点为 v13、levelIndex=2、植物池 10、扎根 4 层。原版 user1.dat 前后 SHA256 均为 10CD4360C11E2CD54FA440A1B05892FE836BE552C71B59C7258D086F1E95719F。临时入口已从最终源码移除；未等待完整战斗胜利，不宣称已完成平衡验证。
- 语言前移到当前阶段，范围按全部界面。自动/中文/English 保存后重启统一应用；原版菜单、图鉴和提示通过玩家自己的 pvz_zh.pak/LawnStrings_zh.txt 接入，新增页面与资源管理页双语。复用 PC 位图字体，内置 Noto CJK 补齐新文案和缺字，许可与来源见 docs/LOCALIZATION.md。
- MuMu 自动模式识别中文，已看到中文标题、主菜单、选项与保存提示；通过语言按钮保存 English 后重启，标题/主菜单图片和闯关入口均恢复英文。成就页翻译键另作适配。
- 最终包恢复第 3 关检查点，中文闯关模式页、选卡页、图鉴目录与豌豆射手长文均显示正常；成就页和资源管理页中文显示正常。恢复后原版 user1.dat 哈希仍相同。SAF 复制交互与全部图鉴条目未穷举。
- 当前包为仓库外 `android-artifacts/pvz-online-run-language-arm64.apk`，27,914,172 字节，SHA256 `41D798AEEB5AC0D1C3D6EA059161F1FE1B38ECD7052B26A2F179A433E16C70D4`；增量原生编译/Gradle 打包通过，开发签名 v2 验证与 16 KB zipalign 检查通过，已更新安装到 MuMu。APK 只内置开源字体，没有原版或 PC 汉化素材。
- 未完成项：PC 出怪/难度/全部增益、推车跨关规则、联机、SAF 用户操作与真实 ARM 手机性能。中文资源检查只覆盖代表性页面，没有逐条遍历全部原版对话。

### 底座记录

- SDK 安装成功，Gradle 能启动，SDL2 编译成功。
- 图像/音频依赖、游戏 native 库与 APK 已构建成功。API 24 使用独立安装目录 `android-native-api24-deps/`，不要误用首次 API 28 的产物。
- 底座包：父目录 `android-artifacts/pvz-online-base-arm64.apk`，14,179,626 字节，SHA256 `03B8E9BBE64BAAC13AE767D7A8C6F59EB3E4916671BC84A0ED0E744E1180DFC6`。开发签名 v2 验证通过，APK 16 KB zipalign 检查通过；运行记录见下。
- Gradle 工程配置已通过。开发包名 `io.github.pvzonline.android`，名称 `PvZ Online Dev`，与社区原版隔离；Java namespace 暂保留底座命名。
- 可重复构建脚本：父目录 `build-android.ps1`，阶段 `Dependencies` / `Native` / `Apk` / `All`。
- MuMu 已升级为 Android 15，9 GB 内存，实例 2；ADB 127.0.0.1:16448，ABI x86_64/arm64-v8a/x86。已安装 ARM64 包、看到资源管理页、通过 ADB 复制本机正版资源、进入主菜单和第一关并观察到已种植的豌豆。OpenSL ES 音频流已启动；没有通过听音确认声音。未检验 SAF 交互导入全流程，也没有把模拟器结果当作真实 ARM 手机性能结论。
- 用户报告种子拖动松手不种植，已实现 Android 拖放修复；增量编译、APK 打包和更新安装通过。在 MuMu 的 1-2 关用向日葵验证：拖到无效土地区域松手取消，阳光保持 50，无悬浮植物；随后拖到草坪松手直接种下，阳光变为 0，无需第二次点击。点选再点格子仍沿用原路径（此轮未另做交互回归）。
- 修复包：仓库外 `android-artifacts/pvz-online-touch-arm64.apk`，14,180,106 字节，SHA256 `B1DC726EFA64BD403A132460A2D51B5055F961F4ED8A4B7ADB0D7BF9E60940B8`。保存了有效/无效落点截图，未将游戏画面上传共享仓库。
- Android 已迁到独立 PVZ-online-android 仓库；现有 PVZ-online/android-port 保留为旧临时副本，后续不再向它同步。任务书见 docs/COLLABORATION.md。
- 未改 Windows 游戏代码、未部署服务器、未打包正版素材。

## 差异地图初稿

| 本项目模块 | Portable 对应/动作 |
|---|---|
| Lawn/Online/NetProtocol.h | 纯固定宽度小端编解码，优先保持协议 |
| Lawn/Online/NetLink.cpp | Winsock/线程/锁改跨平台；保留对外接口与超时语义 |
| Lawn/Online/NetSession | 核对引擎接口、帧计时及 Android 生命周期 |
| Lawn/Run | 迁入 src/Lawn/Run；CMake 当前 GLOB 未包含此子目录，需显式加源文件 |
| Sexy.TodLib | Portable 改为 src/PvzpLib，Tod 类型/函数需要适配，禁止整目录覆盖 |
| Board/Plant/Zombie/Projectile | 按功能挂点迁入，保留 Portable 平台与保存修复 |
| ModText | GDI 路径不可复用；核对 Portable 字体能力后适配 |
| GameSelector/联机对话框 | 加入口、软键盘和触屏按钮；先不改逻辑画布 |

阶段顺序与验收预算见父目录 `ANDROID-PORT-PLAN.md`。
