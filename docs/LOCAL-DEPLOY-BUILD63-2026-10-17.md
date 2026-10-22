# MOD63 本机双端覆盖更新 · 2026-10-17

## 用户授权与范围

本次在上一批公告和完整包交付之后，用户明确选择“把本机电脑版和手机版也更新到MOD63，保留存档”。仅更新实际使用的PC运行目录和当前可连接的Android应用，不改写历史发包、旧分支或以前的备份。

- PC目标：工作区 `runtime/`，主程序 `SexyAppFramework.exe`，附完整资源、x86运行库、中文启动脚本与公告。
- Android目标：MuMu安卓设备-2、Android15、`127.0.0.1:16448`，正式应用 `io.github.pvzonline.android`。本次没有连接实体手机，不宣称实体手机已经更新。
- 更新前双方游戏均未运行。没有强退进行中的对局，没有运行或继续实际存档，也没有进行玩法验收。

## 备份

工作区 `android-artifacts/deploy-build63-20261017-123242/`：

- `pc-runtime-before/`：被本次包覆盖的已有运行文件；不存在的新增文件在 `before.json` 中记录。
- `pc-userdata/`：存在的PC userdata原样备份，包含当前主要档案及其他旧数据根，不用旧备份覆盖现有进度。
- `pc-settings.reg`：当前游戏设置注册表导出。
- `android-before.apk`：更新前已安装的原APK，版本49／online57。
- `android-files/`：Android外部游戏数据目录完整备份，含资源、设置、存档及缓存，共276个文件，逐文件SHA256与设备一致。
- `before.json`、`deployment-result.json`：原文件哈希、目标文件哈希、时间、设备与结果。

PC实际进度位于 `C:/ProgramData/PlantsVsZombies/userdata`，同时备份了存在的其他历史userdata根。未对游戏注册表值或存档内容做修改。

## 安装与核对

- 2026-10-17 12:33完成。PC使用MOD63完整包覆盖；资源相同的文件不重复写入，其余文件保留原件后更新，旧版诊断程序及无关文件不删除。
- Android先核对原APK和新包证书一致，再直接 `install -r` 覆盖正式应用；没有卸载、清数据、降级或手动导入资源。
- 安装后版本63／`0.1-online63.c01ab12b-full`。从设备重新取回的已安装APK与完整交付APK的SHA256一致：`c1b882688cd9721dc45e65c16e193e4ffdbf9a512df5622ad8b0373fc8b52fa6`。
- PC主程序SHA256：`3e9fe77a8a87e92345cd71881cf7a8d0d76df726de186ac7425afb6a8e6ed7f2`，运行目录内全部交付文件与完整包一致。
- 覆盖前后PC存档全部文件哈希一致，Android全部276个外部游戏文件哈希一致。没有回退进度，没有在新版本中启动实际对局触发迁移。

完整包之前已做隔离安装／首次启动、依赖、签名及完整性检查，见[发包记录](PLAYER-FULL-BUILD63-2026-10-17.md)。本批只做安装与保档核对，不冒称完成六人长局、复杂伤害统计或配乐听音验收。

PC可使用 `runtime/启动游戏.bat`，Android使用原图标启动；联机请全员统一63。部署工具为工作区 `tools/deploy-local-build63.py`，其版本前置检查针对本次49→63，不应当作通用重复更新器。
