# PC 战斗快捷键

2026-10-15，MOD_BUILD 49。该批只改本地输入与提示，不抬联机代次，不改玩法数值或存档布局。

| 按键 / Key | 操作 / Action |
|---|---|
| 1–9 | 从左到右选择前九张卡 / Select seed slots 1–9 |
| 0 | 第十张卡 / Select seed slot 10 |
| Q | 拿铲子 / Pick up shovel |
| ESC | 取消当前选择；无选择时沿用菜单行为 / Cancel selection; otherwise open the menu |
| F5 / F6 | 僵尸 / 植物血量数字开关 / Toggle zombie / plant health numbers |
| T / E | 快捷消息 / 植物表情 / Quick messages / plant emotes |
| G | 送阳光，选择态按 1–6 指定队友 / Gift sun; pick a teammate with 1–6 |
| 按住 V / Hold V | 观战，观看中按 1–6 选择席位 / Watch a teammate; switch seats with 1–6 |

主数字键与 NumLock 开启的小键盘均可选卡；键位绑定卡槽顺序，不绑定植物种类。按键只拿卡/工具，之后点击草坪执行。ESC 可取消；同卡重复按保持选择，切到不可用的卡会保留当前选择并沿原版规则提示。阳光、冷却、升级条件和铲子解锁不变。

打开消息、表情、送阳光或观战选择态时，数字键先处理这些功能，不同时选植物。长按、暂停、模态框、对白/过场、鼠标按住期间和特殊光标中不会切卡。普通种子栏的冒险、闯关、生存接入；传送带、特殊小游戏与花园工具仍用原来的操作。

实现位于 Board 的键盘入口与现有 SeedPacket/PickUpTool 路径；卡片角落和铲子旁用游戏字体标键。Board 只新增方法，无对象字段或快照格式变化。Win32 KEYDOWN 的重复标记保留到焦点路由，WM_CHAR 调试路径不再重复执行数字/Q 的战斗按键。

验证：MSVC x86 Release 构建通过。仓库外的小型 C++ 检查直接抽取本次 Board 与 WidgetManager 输入函数，使用假的卡片/工具动作依赖，检查 1/9/0、小键盘、不可用卡保留、Q、长按与协作/观战优先级、F5/F6、暂停和手势门禁；全部通过。它验证输入入口，不替代真实引擎种植或多人实战。Windows computer-use 连续两次初始化报 `failed to write kernel assets ... os error 3`，本轮未取得真实窗口按键/截图验收证据；没有改用 PostMessage 冒充真实按键。未重跑 buff。

最终构建包含 PC 同期的大喷菇 Build 49 更新。Android 仍使用固定 Build 46，本轮未做 Android 规则同步。当前已运行的其它工作树进程保持原样；PC 玩家需重新启动更新后的程序才能使用新键位。
