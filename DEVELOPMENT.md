# Android 开发入口

源码基线、构建身份见 PORTING-STATUS.md；合作分工见 docs/COLLABORATION.md；路线见 docs/ANDROID-PORT-PLAN.md。

## 当前本机构建

`tools/android/build-android.ps1` 使用仓库旁的 SDK、vcpkg、Gradle 与依赖缓存；它已包含项目 API 24 Release triplet，不使用 vcpkg 默认 API 28。

```powershell
# 玩法/C++ 修改：增量原生编译，然后打包
./tools/android/build-android.ps1 -Stage Native
./tools/android/build-android.ps1 -Stage Apk
# 只改 Android Java 或 XML：仅打包
./tools/android/build-android.ps1 -Stage Apk
```

这是当前 Windows 主开发机构建入口，尚未成为跨机器自动安装脚本。其他机器需准备同版本工具，设置脚本的 JAVA_HOME 路径，并准备父目录 android-sdk、android-vcpkg、android-build-tools/gradle-8.12。源码 SDL2 2.32.10 放在 sdl2-src；其目录被忽略，可从 libsdl-org/SDL 的 release-2.32.10 下载。所有工具版本见 PORTING-STATUS.md。

先做编号审查不需要这些工具。资源管理页开发只需同版本 JDK/SDK/Gradle、SDL Java 源，以及本机已编译的 jniLibs；不得把原版资源或签名密钥放入 Git。

构建脚本会修正 SDL 2.32.10 Android 表中的 NumLock 空映射，并在该源新于缓存库时增量重编 SDL。不要只替换 libmain.so 后继续使用修正前的 libSDL2.so，否则小键盘无法切换数字状态。默认键位及有限验证见 [快捷键结果](docs/KEYBOARD-CONTROLS-STATUS.md)。

## 当前 MuMu

Android 15 实例编号 2，当前 ADB 地址 127.0.0.1:16448（每次通过 MuMuManager info 获取，不能长期写死）；ABI 列表 x86_64,arm64-v8a,x86，能安装当前 ARM64 包，运行依赖模拟器翻译能力。

开发包名 io.github.pvzonline.android；活动类仍为 io.github.wszqkzqk.pvzportable.PvZPortableActivity。正版资源只放本机模拟器 `/sdcard/Android/data/io.github.pvzonline.android/files/`，不入共享仓库。
# 玩家交付记忆（用户定案，2026-10-16）

以后打包默认完整可用：Android完整APK内置资源、汉化及运行依赖，安装打开即可玩，首次启动自动准备资源；不得要求玩家手动导入、补文件或配置环境。玩家按没有技术经验设计，更新保留存档，并验证全新安装启动。只有用户明确要求更新包才交增量包。此用户要求覆盖旧文档默认不带资源的玩家交付口径；工作区根`AGENTS.md`保存双端共同约定。
