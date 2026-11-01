# MOD104 观战带宽对比测试

在作者 MOD104 `52d7ebca` 的 test 分支上，同时保留两条可选择的观战路径。默认启动仍为作者方案。

```powershell
# 作者绘图预测，测试端启用后台接口与底线死亡
SexyAppFramework.exe -control=AuthorA -watchengine=author -bench-edge-kill=1
# 我们的 PWV2 自定义状态包 + 隔离进程本地模拟
SexyAppFramework.exe -control=LocalA -watchengine=local -bench-edge-kill=1
```

同一测试房间的两端应选择相同 watchengine。`-bench-edge-kill=1` 在僵尸触发原生底线检查时执行无掉落死亡，阻止漏怪转移与失败；默认关闭，只用于本次测试。它不提供提前秒杀或资源修改。普通启动不启用底线死亡。

我们的路径保留原先的每 5 秒状态同步、种植/铲除触发更新且最小间隔 1 秒、配置首次发送并按版本确认、逐对象小误差保留本地位置，以及隐藏观战场上阳光的处理。接收端实际恢复对象并运行战斗逻辑；不是只统计一个未使用的数据包。隔离观战子进程只使用临时存档目录，关闭观看端时会一起结束。

本次用普通版、标准 ×1 的第一关，两个客户端不种植物、不手动提前催波，正常开始出怪，僵尸走到原生底线死亡。两套方案依次运行，编译与游戏测试不同时进行。每次均使用隔离测试档案和作者云服务器房间。主比较为作者 P2P 持续观战与我们原有 TCP 中继状态同步；传输路径差异属于当前两套实现的一部分。

运行命令：

```powershell
python -X utf8 benchmark_watch.py --exe SexyAppFramework.exe --engine local --tag MeasureLocal --out evidence/local
python -X utf8 benchmark_watch.py --exe SexyAppFramework.exe --engine author --tag MeasureAuthor --out evidence/author
```

`result.json` 仅在本次观察循环结束后生成，应检查 `end_reason == level_complete`。samples.jsonl 保存逐次采样，startup.json 保存初始关卡、模式、难度、种子和卡槽等状态。失败或中断的尝试不纳入整关平均。

计量从订阅前开始，包含初始化、握手、正常等待首次出怪及整关直到完成的时间；同时记录实际开始操作后的战斗时长。平均速率 = 此时间段累计实际字节数 / 时间段秒数。

- 作者观战上传：成功写入 socket 的观战 TCP 报文 + UDP 报文（包括直传探测、心跳与协议开销）。
- 我们观战上传：成功写入 socket 的 BOARD_SNAPSHOT 报文，包含 TCP 游戏帧与 flow 封装；单独的订阅、配置 ACK 等控制报文包含在连接总量中。
- 连接总上传/下载：TCP + UDP 的所有 socket 字节，包含普通联机消息。
- 压缩包大小和排队量另列，不冒充实际网络发送量。
- 不计 IP/TCP/UDP 底层头部、链路层开销及系统 TCP 重传。不是网卡抓包总流量。

各轮种子和实际怪数会在结果中记录；同模式、同难度并不代表随机怪物组成完全相同。本次不种植物，因此不代表有大量植物、子弹和特效时的带宽，也不代表不同家庭 NAT 下都能建立 P2P。作者默认关闭 P2P 时的单次快照不能与持续更新直接混算。

新增代码与本次移植均由 AI 完成，仅经过记录中的初步测试。未自动提交或推送。
