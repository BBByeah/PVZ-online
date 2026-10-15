# 接力怪标记复查与修复断点

2026-10-16。用户反馈“传递怪标记没有实现，检查”。**双端源码修复及PC真实渲染/读档定点已完成；没有覆盖玩家正在运行的程序，没有安装Android新包。** PC修复提交 `5224048366493432daab49c1238a9062327d9f0c`，Android固定来源同步此提交，仍为Build53。

- 当前PC HEAD 1ae3ec3、Android HEAD 7ba1598，Build53；两端源码均有 AddRelayedZombie→MarkRelayedZombie、父单位继承和观战标记位。
- 已确认原本箭头在 Zombie::Draw 中先于本体动画绘制，易被本体/装备/后续其他实体覆盖；当前改为 Board::DrawRelayedZombieMarkers 最终叠加层，按实时GetZombieRect定位。
- 已确认标记集合在既有存档中完全未保存，读档后丢失。当前PC追加可选RLY1尾块，Android新增可选v4块106，均验证ID有效性和数量，旧档默认无标记。
- PC Release增量编译与Android Native增量编译已通过（含存档改动）；日志在仓库外 android-artifacts/relay-marker-fix/。没有修改联机消息或Go中继，不抬Build。
- 已将可复用真实渲染探针提交到PC `tools/relay_marker_probe.cpp`；同目录probe.rsp/.exe为本机链接入口。独立产品名PvZRelayMarkerProbe隔离数据，不操作玩家47972进程，不覆盖runtime可执行文件。初始化/命令行适配问题已解决，最终探针退出0。
- 当前玩家PC进程仍暂停在对局中，绝不能杀掉/覆盖它来验证。current-pc.png只用于看当前状态，1902×1326非1:1，不是布局验收证据。
- Android RunBuffs.cpp/.h 是其他agent当前修改，禁止添加进本批提交。PC tools/symbols/同样不属于本轮。

## 运行证据

- `fixed.bmp` 与 `old-order.bmp` 是游戏原生MemoryImage的800×600离屏渲染，不经桌面DPI缩放。内容含普通自产怪、接收路锥和接收巨人；旧顺序由同一实际模型渲染路径复现。该测试直调接收入口，不代表新一轮端到端网络实战。
- 像素区(400,100,200,400)内RGB(255,215,70)计数：旧顺序0，修复后184（两枚各92）；`pixel-check.json`。证明箭头确实被遮住，修复后确实可见。
- `probe-run.log`：普通怪marked=0，接收路锥/巨人marked=1；`CHECK inherited=1 saved=1 loaded=1 retained=1 tail=1 old_compatible=1`。独立同场保存含继承小鬼，读回3个标记；移除可选尾块的旧档仍可读，标记为空。
- `relay-save.dat` / `old-save.dat`全为独立探针数据。没有注入、替换或恢复用户原档。
- Android本轮仅Native编译，未重新验证Android存档读写、观战或完整双端传递。保留观战缓存坐标画法，只修正不再准确的同位说明；此次可见性修复针对本机草坪最终叠加层。

## 接续与交付边界

1. 本轮只提交Board/Board.h/Zombie/SaveGame、观战注释、固定来源和文档，不提交其他agent的RunBuffs.cpp/.h；其当前火力20%变更尚在工作区，不能在新APK中假称来自固定PC来源。
2. 本轮未产生新APK；此前候选不含此修复。下次整合buff后统一打包，再做一次Android接收怪和保存重载定点即可，不需要全套冒烟。
3. PC新产物在 `re-plants-vs-zombies/build-rel-x86/SexyAppFramework.exe`。`runtime/SexyAppFramework.exe`仍是正在运行的旧程序；不能杀用户的暂停对局来覆盖，也不能声称现有进程已修复。玩家正常结束对局后再更新运行产物。
4. 已经用旧版保存、丢失来源标记的档无法反推“哪只是接力怪”；新接收怪和新标记保存开始保留。禁止把所有在场僵尸一律标成接力怪。
