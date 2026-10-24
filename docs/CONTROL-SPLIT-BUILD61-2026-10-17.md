# Build61 · 僵尸控制拆分 · 2026-10-17

## 已实现

- ID5改为免硬控；新增ID8软控缩时（持续时间减半、幅度不变），各最多1层，常规候选与第二屏替换均过滤已满项。
- 保留mControlImmune字段及棋盘存档布局，含义缩为硬控；普通减速不再查该标记。冰冻、黄油、魅惑、换道、击退继续拦截。
- SoftControlDuration按关底标记或软控词条择一减半；冰缓、地刺、三叶草、腐臭、霜甲、仙人掌直接写计时器入口全部接入，刷新取较长剩余时间。正时长向上取整，避免1帧变为免疫。
- 三种关底BOSS均可减速，冰车特殊位移分支应用原生冰缓系数及腐臭/霜甲速度倍率；不只改变动画。普通冰车固有冰缓免疫不改。
- 橄榄球关底BOSS按标记豁免Ghostwalk，接力后同样需停下啃食；每口200及狂食链未调数值。
- 共享词条数9；开局消息43字节、投票ID校验扩到0..8、缓存重连使用扩容结构。两端Build61、协议1，Android保留onlineGameReady=false。
- 检查点v18按源版本读取8或9条；v14..17旧免控转换为两条各1层，后续字段不移位。PC棋盘快照v5及Android便携v4布局无需改变。

## 已验证

- PC x86 Debug构建及Android API24 arm64 Release原生构建成功。PC首次链接报既有CoopPlay对象COMDAT缓存错误，删除该单个构建对象并重新生成后成功；未改协作源码来绕过错误。
- PC链接实际游戏对象的37项无窗口机制探针全部通过：硬控/软控独立、普通和寒冰菇残留冰缓时长、刷新/1帧/奇数边界、三种BOSS及叠加、普通车型、橄榄球啃食索敌与200伤害、冰车实际位移、实际腐臭/三叶草施加、层数封顶与100轮候选、v18往返、v17/v14迁移及后续字段、43字节快照和投票ID校验。
- 分别编译PC/Android协议头的消息探针，九条层数开局与投票消息逐字节一致。
- 检查新按钮别名“抗性”两字均在原艺术字表，不修改图集或字体。
- 测试数据仅写入tools/audit/control-split-data，未读写真实玩家存档。未覆盖runtime、未关闭玩家游戏、未安装或发包。

## 未验范围

完整对局、六席位跨端实战、实际掉线重连/接力、Android设备运行、投票及查看器视觉验收未执行。编译和机制探针不能代替这些验收。本批只实现已确认三项，不实现其他候选Buff，也不调BOSS血量、狂食或出现次数。用户若要求发包，按根AGENTS.md交付完整可用包。

## 本地证据

- tools/audit/control-split-pc-build-retry.log
- tools/audit/control-split-android-build.log
- tools/audit/control-split-pc-build-final.log / control-split-android-build-final.log（最终源码双端编译成功）
- tools/audit/control-split-probe.cpp / control-split-probe-result.log（37 PASS，0 FAIL）
- tools/audit/control-split-wire.cpp（分别使用两端协议头）
- tools/audit/control-split-before（改动前相关文件快照，保留原有未提交改动）
