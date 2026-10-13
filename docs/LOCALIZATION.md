# Android 中英文资源与语言选项

本阶段覆盖目标为原版菜单、图鉴、提示、成就、Android 资源管理页和新增 mod 页面；不只翻译闯关入口。Windows 的 GDI/注册表实现没有迁入。

## 使用

在原版“选项”中点击语言按钮，依次切换“自动 → 中文 → English”。选项保存在 `userdata/android-language.txt`，值为 0/1/2；重启游戏后统一应用，避免已缓存的字体、图片和新文字混用。自动跟随系统首选语言。

原版中文美术和文本复用玩家本地 PC 的 `pvz_zh.pak` 覆盖包，与 `main.pak` 放在同一资源目录。资源管理页支持从包含两者的 ZIP 或目录导入。中文文本 `properties/LawnStrings_zh.txt` 可以来自覆盖包；在原版和本地 properties 加载后再覆盖，避免 default.xml 的英文值盖回中文。

没有覆盖包时保留英文界面，日志和语言选项提示需要补包。仅提供 LawnStrings_zh.txt 无法切换原版图片。包中字体需为 UTF-8 字符映射，不能直接用依赖 PC 特定 EXE 编码的旧汉化包。

## 字体

原版资源字体继续使用 PC 汉化包的位图图集。新增 mod 页面使用 APK 内置 Noto Sans CJK SC；位图字体找不到字形时，相关文本回退到此字体，并保留裁剪与宽度计算。字形按需缓存，每档字号最多 512 个；英文仍走原字体。

- Noto 来源：[notofonts/noto-cjk](https://github.com/notofonts/noto-cjk)，提交 `f8d157532fbfaeda587e826d4cd5b21a49186f7c`，文件 `Sans/OTF/SimplifiedChinese/NotoSansCJKsc-Regular.otf`。
- 字体 SHA256：`2C76254F6FC379FDDFCE0A7E84FB5385BB135D3E399294F6EEB6680D0365B74B`；SIL OFL 1.1 许可随字体放入 APK 的 assets/fonts/LICENSE。未改字体文件。
- 栅格化库：[nothings/stb](https://github.com/nothings/stb)，提交 `2c980bb59875b0d32144a71867fbdebb2f77cd20`，stb_truetype.h，保留原文件许可与 LICENSE。
- stb 只读取随 APK 交付的固定字体，不读取玩家导入的字体文件。其他平台如需运行此后备字体，须在资源目录提供同名 fonts/ 文件。

正版美术、主资源包和 PC 汉化覆盖包均不随源码或 APK 分发；内置的开源字体与它们分别管理。

## 覆盖与验收边界

Portable 的成就页原先用英文句子查询属性，与 PC 的 ACHIEVEMENT_* 键不同；已接入原有中文键，包含成就提示和奖励页。后续新增功能必须提供双语文案，并复用本模块，不能再新增 GDI 路径。

只验收本次相关页面与一次语言重启切换，不逐关遍历所有对话。无法保证任意第三方汉化包具有相同文件与键；当前兼容依据是开发机现有的 PC 覆盖包。图鉴长文、字幕等沿用 Portable 的 UTF-8 换行；实际页面检查结果记入 PORTING-STATUS.md。
