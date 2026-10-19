# Build58：天降夜晚/迷雾不生效修复

玩家报告“夜晚降阳光buff没生效”。PC和Android均存在相同故障，已同步修复并打出带资源、汉化和依赖的完整包。

## 原因与修正

`NewGame`先创建草坪和调用`InitLevel`，随后才处理待选增益。夜晚没有天降时，初始`mSunCountDown=0`；在该草坪上拿到天降后，`UpdateSunSpawning`开始运行，但旧代码先`--`再检查`!=0`，第一次变成-1，此后一直负数，永远无法落阳光。该负值还可能保存在局中快照里。

双端`Board::UpdateSunSpawning`现在只递减正倒计时，并把非正值视为到期。因此在夜晚/迷雾草坪建好后选到天降能够立即进入正常掉落循环，旧档里的负倒计时也会自动恢复。常规正倒计时、夜晚无词条不掉阳光、结算及特殊模式门禁不变；天降仍为2倍速率，旧档多层不额外叠乘。

双端版本升至MOD_BUILD58，Protocol1和关间checkpoint17不变，未改实体布局或存档格式。Android基线使用明确标注的工作树覆盖层，不把未提交修复冒充已经提交的PC基线。联机玩家请统一更新58。

## 验证

探针直接提取两端完整生产`UpdateSunSpawning`函数，仅替换类限定名以挂在真实PC Board派生类上；使用真实LawnApp、RunState及夜晚/迷雾判定。最终`AddCoin`调用被捕获以验证生成请求的类型、位置及来源，因此这些是定点机制验证，并非完整画面/实际落币动画或网络整局验收。

- 修复前：26项中8项失败（双端各4项），复现拿词条后不生效、迷雾零计时和旧负计时无法恢复。
- 修复后：26项全部通过，覆盖夜晚、迷雾、0/负/正倒计时、连续两次掉落、2倍速率、旧档多层、无词条、关卡结束及坚不可摧限制。
- PC Release、Android Native和Release APK构建通过，Android版本元数据检查、双仓`diff --check`通过。APK v2签名和16KB对齐通过，完整包逐个ZIP校验、8个内置资源哈希及原生库一致性检查通过。
- 本轮未重复整个首次安装流程；完整包自动准备及已有资源/存档保留沿用刚完成Build57的首次启动验证，启动流程未修改。本轮未安装到用户设备、覆盖本机运行目录、连接玩家房间或改变玩家存档。

## 完整交付文件

位于工作区`dist/`：

| 文件 | 字节 | SHA256 |
|---|---:|---|
| `PvZ-Online-Build58-Full-20261016-PC.zip` | 60,808,855 | `100609b1796c050368a90d8a671a730013830e3a8fbab4831fb230e504eff98f` |
| `PvZ-Online-Build58-Full-20261016-Android.zip` | 78,784,457 | `611208557b2c8f432d696c6a1741f46f9cfbe2a060352367cc0bd3f40f339c39` |
| `PvZ-Online-Build58-Full-20261016-Both.zip` | 139,597,344 | `1fe7440c3467ec3cfdf425152f443a5186e87f28f9899541adf8a3934f3ea514` |

直接安装APK：`dist/PvZ-Online-Build58-Full-20261016/Android/PvZ-Online-Build58-Full-20261016-arm64.apk`，包名`io.github.pvzonline.android`、versionCode49、版本名`0.1-online58.c01ab12b-full`，沿用现有测试签名。PC解压点启动、Android安装打开即可，无需另导资源。旧Build57产物保留作历史。

证据：`android-artifacts/skyfall-fix-20261016/`内保存前后结果、函数源码和构建日志；`android-artifacts/full-android-build58-20261016/delivery-manifest.json`记录最终二进制和所有完整包哈希。探针生成脚本为`tools/audit/make-skyfall-probe.py`，完整打包脚本已按当前MOD_BUILD动态生成名称，避免覆盖旧包。
