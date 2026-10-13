# 两人协作与任务交接

## 共享仓库

最终采用单独 Android 仓库，用户新建空仓库后提供链接。临时同步位置为 `https://github.com/Not-a-Different-Name/PVZ-online/tree/android-port`；不要将此 Portable 底座分支整支合入 Windows 的 pvz-online。以下命令暂使用临时位置，新仓库就绪后替换 origin URL。

GitHub 操作：打开上述仓库 → Settings → Collaborators → Add people → 输入同伴 GitHub 用户名。对方接受邀请后可推自己的分支。仓库公开或私有由所有者决定；源码许可证与归属说明都必须保留。

本地推送目标必须是用户确认的 Android 仓库，不能使用 Portable 上游地址。共享源码不包含 SDK、NDK、缓存、原版素材、签名密钥或 APK。Windows 参考头文件来自下述明确提交，只供对照、不加入编译。

### 分支与 PR

- `android-port`：双方认可的 Android 整合基线。
- 主开发者：`port/core`，负责 C++ 玩法、网络、构建及最终 APK 整合。
- 同伴任务 A：`collab/enum-audit`，只负责编号兼容报告。
- 同伴任务 B：`collab/import-i18n`，只有确认接取后才开始，负责资源管理页双语。
- 每次开始从最新 android-port 拉分支，一项任务一个 PR。PR 写改了什么、依据/验证、仍需主开发者处理的问题。
- 同伴不直接推 android-port；主开发者查看差异后合入。若同伴需要修改约定之外的文件，先在 PR/Issue 中解释原因并调整分工。

对方接取 A 的命令（把仓库 URL 换成实际链接）：

```powershell
git clone --branch android-port https://github.com/Not-a-Different-Name/PVZ-online.git pvz-online-android
cd pvz-online-android
git switch -c collab/enum-audit
# 完成报告后，只提交报告文件
git add docs/compatibility/enum-audit.md
git commit -m "docs: audit PC and Android enum compatibility"
git push -u origin collab/enum-audit
```

然后在 GitHub 创建 PR，目标 android-port。A 不要求安装 Android Studio/SDK，也不要求构建 APK。

## 同伴任务 A：枚举与协议编号审查（建议首先接取）

### 输入

- `docs/reference/windows/ConstEnums.h`
- `docs/reference/windows/NetProtocol.h`
- `src/ConstEnums.h`
- 参考来源：Windows 提交 `7a756a2db2ef88b10a08ec4d5c34900818a4995c`，Portable 提交 `141c1476dd462ac3526f796e303177616f4de6bc`。Windows 参考文件保留其 CC0 许可文本。

### 工作

1. 对照 SeedType 和 ZombieType 的实际数值，区分同名同值、同名异值和仅 PC 存在。列出 NONE、NUM_* 及附近特殊/占位项，不能只计算列表长度。
2. 结合 NetProtocol 确认哪些消息传植物/僵尸编号、位宽及解码验证；特别标记装甲植物头等新增类型。
3. 找出 Portable 里按 NUM_SEED_TYPES/NUM_ZOMBIE_TYPES 建立的定义表、名字表或数组，以及收到未知值后的风险入口。只列重要入口，不穷举无关 UI。
4. 给出建议：保留 PC 编号或映射编号；哪些新增条目必须在接收前实现。报告明确区分已确认与推测。

### 交付与范围

- 只新增 `docs/compatibility/enum-audit.md`，含来源、对照表、相关文件位置、待主开发者处理项。
- 不修改 ConstEnums、Board、Zombie、Plant、NetProtocol、构建文件或资源。
- 完成标准：所有实际植物/僵尸编号有对照；新增类型与计数项标明；数字依据能追到输入文件。
- 如需辅助脚本可附在 `tools/audit/`，但不建立大型测试框架、不要求运行游戏。

## 同伴任务 B：资源管理页中英双语（可选的小编码任务）

### 目标与范围

资源管理页当前有硬编码英文。将其用户可见文字提取为 Android string resources，提供中文和英语，按 Android 系统语言选择；保持原来的 ZIP/文件夹导入、存档导入导出和启动流程。

允许修改：

- `android/app/src/main/java/io/github/wszqkzqk/pvzportable/ResourceImportActivity.java`
- `android/app/src/main/res/values/strings.xml`
- 新增 `android/app/src/main/res/values-zh/strings.xml`

不要改包名、namespace、签名、Gradle、Manifest、PvZPortableActivity、C++、底座已有版权提示的含义或资源合法来源要求。任务期间主开发者不改上述资源管理文件；遇到确需变更范围的地方先沟通。

交付：一个 PR，附一张中文页面截图和一张英文页面截图；一次 Java 编译与一次 ZIP 或文件夹导入即可。不重编 C++，不做全部存档格式回归。此任务需要准备 Android 打包环境；先做 A 可以避免对方花时间装整套工具。

## 当前共享基线的能力

2026-10-13 已生成原样 Portable 底座 APK（仅开发包标识/构建配置适配），在 MuMu Android 15 安装启动并读入本机资源。尚未迁入 PvZ Online 闯关、增益和联机功能。APK 位于开发机仓库外，不把它误称为完成的手机版 mod。

官方协作操作参考：https://docs.github.com/en/repositories/managing-your-repositorys-settings-and-features/repository-access-and-collaboration/inviting-collaborators-to-a-personal-repository
