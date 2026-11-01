# MOD104 test：后台控制与诊断

基于作者 MOD104（52d7ebc），保留作者的绘图快照观战、战斗规则和默认关闭 P2P 的设置。本包不是 ByeahFeature 的本地模拟观战版。代码尚未提交或推送。

## 启动

- `启动游戏.cmd`：普通启动，不开放控制管道，沿用作者存档位置。
- `双端后台演示.cmd`：需要 Python 3，自动创建两个隔离测试实例，进入普通、标准 ×1 联机局并让 B 观战 A。音乐关闭，默认不启用 P2P。
- 游戏本身不需要 Python。解压整个文件夹运行；包含 `pvz-lan-host.exe` 和运行库。

演示通过作者云服务器连接。网络故障会明确报错；不要把控制命令超时理解为操作没有执行。脚本默认将窗口放到左侧副屏，可用 `window.position` 调整。正常联机游戏会继续推进，也可能正常失败；本包不自动刷资源、秒杀或免死。

## 控制命令

在包目录运行 PowerShell，以下 `DemoA` 是实例名：

```powershell
python control.py DemoA launch
python control.py DemoA state
python control.py DemoA ui.click x=400 y=300
python control.py DemoA ui.button label=返回
python control.py DemoA ui.text text=测试玩家
python control.py DemoA ui.key key=27
python control.py DemoA team.vote choice=0
python control.py DemoA plant slot=0 col=0 row=2
python control.py DemoA collect coin_id=123456
python control.py DemoA shovel col=0 row=2
python control.py DemoA pause enabled=true
python control.py DemoA pause enabled=false
python control.py DemoA menu.open
python control.py DemoA menu.close
python control.py DemoB watch.start seat=1
python control.py DemoB watch.refresh
python control.py DemoB watch.stop
python control.py DemoA p2p.set enabled=true
python control.py DemoB p2p.set enabled=true
python control.py DemoB screenshot path=watch.bmp
python control.py DemoA window.position x=-1880 y=120
python control.py DemoA close
```

卡槽、行、列从 0 开始，席位从 1 开始。`coin_id` 取自 `state.board.coins`，投票 `choice` 取自 `state.team.choices` 的 id，-1 表示弃权。种植、铲除、拾取按现有游戏输入处理，不绕过资源、冷却或格子规则。

创建档案、创建/加入房间、模式/难度、接受开局、选植物和增益、选卡和准备，使用 `state` 返回的弹窗、按钮与可选项配合 `ui.*` 完成；完整正常开局示例见 `demo.py`。自绘的准备按钮使用游戏画布坐标。`state` 中 board 是本机真实战场，network 是观战和连接状态，不是观战画面的对象清单。

作者 MOD104 的普通暂停菜单会同步暂停。本接口复用此机制；没有合入 ByeahFeature 的“只有 P 才全队暂停”改动。

## 请求与安全边界

只有使用 `SexyAppFramework.exe -control=DemoA` 启动才开放 `\\.\pipe\PVZ-MOD104-test-DemoA`。实例名限 32 个 ASCII 字母、数字、下划线或连字符。命名管道仅当前 Windows 用户可访问，拒绝远程客户端。

协议为一行 UTF-8 JSON：`{"id":"唯一编号","op":"state"}`；响应含同一 id、ok，以及 result 或 error。请求最大 64 KiB。客户端读完整行后发送单字节 ACK（0x06）；不要自行省略 ACK。

会打开模态界面的 `ui.click/ui.button/ui.key/menu.open/menu.close/pause` 返回 queued 和 command_id，只表示排队成功。随后用 `command.result command_id=编号` 查询执行结果；模态操作可能要等对应弹窗关闭才结束。最多保留最近一批 128 条结果。超时不要盲目重发种植等修改操作，先查询状态。

操作全部在游戏主线程执行；不使用系统鼠标移动、全局输入注入或进程内存修改。截图仅保存游戏窗口客户区 BMP；渲染器或系统可能限制最小化窗口截图，建议保持窗口可见。

为了能在后台持续观战，启用控制接口的实例不会因窗口失焦自动退出观战；最小化、暂停和退出仍按原有流程处理。普通启动没有这个例外，也不会改变原来的鼠标操作路径。

隔离存档：`%ProgramData%\PVZ-MOD104-test-实例名\`，注册表设置也按实例隔离。普通启动仍使用作者原存档位置；演示不会复制或覆盖原存档。

## 日志与灯色

每次启动持续写入 `logs\实例名\`（普通启动为 `logs\player\`）下独立 UTF-8 JSONL。单文件 10 MiB，每个目录最多 10 个文件；每条事件及时刷新。包含启动、场景/关卡、命令结果、观战目标、连接状态和每 5 秒一次的 observer_totals。作者原有 log.txt 仍由作者逻辑维护。

观战顶部小灯与 state.network.p2p 使用相同来源：

| 状态 | 灯色 | 含义 |
|---|---|---|
| idle / disabled | 灰 | 未观战或 P2P 关闭 |
| connecting | 黄 | 正在连接，尚未收到有效直传画面 |
| direct | 绿 | 当前目标、当前关卡收到完整且有效的 P2P 画面 |
| fallback | 橙 | 直传失败或过期，使用作者快照路径 |

关闭 P2P、换人、换关和退出不会沿用旧目标的有效直传标记。对端退出导致订阅结束时显示 idle；不把打洞尝试或仅握手成功当作绿灯。

`observer_totals.message` 是 JSON 字符串，解析外层后再解析 message。计数是本次启动以来的累计值，相邻记录之差除以 interval_ms 可计算速率：

- raw / packed / captures：采集帧的压缩前、压缩后字节及数量。
- queued：交给发送队列的画面字节，**不是实际带宽**。
- tcp_observer_tx：实际写入 TCP socket 的观战报文字节；tcp_tx_all / tcp_rx_all 为整条连接全部流量，含非观战消息。
- udp_tx / udp_rx：实际 UDP socket 收发字节，含探测、心跳和画面；不含 IP、UDP 或 TCP 底层头部与重传开销。
- complete / displayed：组装完成帧 / 校验并接受用于显示的帧；dropped 为已插桩的重组替换与队列溢出；invalid 为解码/绘图准备失败；crc_failed 为直传帧校验失败。并非抓包级的所有网络丢包统计。
- rtt_ms 是直传 RTT；picture_age_ms 是最近有效画面年龄，-1 表示当前订阅尚未收到有效画面。

不记录连接令牌。报告问题时附整个实例日志目录，以及复现步骤和具体时间。
