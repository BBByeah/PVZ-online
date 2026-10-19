# Build37 以来逐提交索引

## 本轮收口另列（不混入下方固定快照计数）

- `1d341ab`：H/J常用血量键、VK大小写碰撞修复、真实键路由渲染检查及版本总览。
- `2eb8646`（并行agent）：植物候选跳过/重摇的跨屏冷却补齐，纯内存记账；Android尚未迁入，非本轮键位代码。
- `af5074f`：以上排1/2/0验收日常选卡，明确小键盘仅可选兼容，9项定点通过。

### 2026-10-16 续（Build54/55，进入分支但晚于上方两段快照）

- `b48b83d`：文档——补入收口时并行提交的植物跨屏冷却与历史快照边界。
- `caf018e`：选择中重摇再续档保留未选奖励，重摇钱包事务化（保存失败不扣款）。
- `5c0d110`：通用词条三处回调——火力+25%、急袭×0.8/层、速种×0.85/层（用户令）。
- `c3c6214`：难度五连改批（MOD_BUILD 54）——巨型Boss三档默认末关+场景3、完整开局三关每关+1增益屏、移速成长高级选项（×1.03/×1.05每场景复利）、一二场景气球/蹦极降权、僵尸buff默认1条；START_LEVEL 36→37字节，检查点停v17。
- `6987b53`：补 `c3c6214` 漏提交的 `Plant::RefreshRunMaxHealth` 定义（否则远端编不过）。
- `b690bc6`：协作交易与准备门收口（MOD_BUILD 55）——主动送阳光等到账回执、取消借用释放名额并断线重试、每次新关/重试独立readyEpoch、无尽保留植物生命词条同步最大血量；Android同步PC54难度字段并修首领档2被清零；START_LEVEL 37→41字节、SEEDS_READY 3→7字节。
- `16cf75d`：文档——记录Build55产物哈希与MuMu玩家数据保留。
- `ef8abdd` / `cbb7e7a`：文档——双端UI改版总计划（Build55）。
- `43a72a1`：UI第一批——房间页六席概览+紧凑准备/重连状态条，双端共用 OnlineUiDraw/OnlineRoomLayout。
- `cb89fc6`：UI第一批小修——保留已连接房间页不自动关闭、初始帮助跟随语言。
- `47a9ed9`：文档——UI第一批部署收尾与产物哈希。
- `2e3b72e`：UI第二批——战斗常驻消息/表情/阳光三入口与同页借用（CoopHud/CoopPlayDialog），收起保留申请、取消等回执、PC鼠标观战返回。

2026-10-16。配合[变更总览](11-Build37以来变更总览.md)阅读。以下只列已进入当前分支的提交，不包含未合并工作树或未提交代码；提交说明是历史记录，旧数值/暂缓项不能覆盖总览中的当前规则。本次H/J修复见总览收口记录。

## PC：Build37首笔 c900e37 至本轮进入时 5224048

共161笔，含同代追加、修复、文档及构建；每笔均可用git show <hash>查完整差异。

| 提交 | 日期 | 原始说明 |
|---|---|---|
| c900e37 | 2026-10-10 | 无尽批 1：多人无尽核心（MOD_BUILD 37）——RunState 无尽档 + 协议 24 字节 + 入口链 |
| b63fdb3 | 2026-10-10 | 无尽批 docs：04 玩法表/版本链补 36→37 + 03 §5.63 批次与验收 + 07 无尽曲线初值 + README 模式表 |
| 5f2285b | 2026-10-10 | README 顺位乘数行修正：末两席同为 ×2（批七口径，与 docs/04 §一 对齐） |
| d56a9d0 | 2026-10-10 | 无尽续草坪 1：过关不清草坪（原地续关，MOD_BUILD 37 批内补） |
| 3ba1854 | 2026-10-10 | 无尽续草坪 2：docs（03 §5.63 同日追补 + 追补验收 ⑪-⑬；04 换关时序/流水线两处无尽例外注） |
| 38c3de4 | 2026-10-10 | 滚爆修复：二次爆判定旗标改弹体 mDamageRangeFlags（旧值缺 DAMAGES_GROUND 位，地面僵尸全被过滤，只剩气球挨得着）+ 圆心 Y 移弹体身体中心（旧粒子锚只剩 ~3.5px 裕量）。协议/MOD_BUILD 不动 |
| f730b88 | 2026-10-10 | 滚爆修复 docs：03 §5.62 补修复记录（根因两处 + ⑧ 复验口径） |
| 8843bc0 | 2026-10-10 | 全局降权：1★/2★ 全局条权重降至原来一半（用户令）。新旋钮 RUN_GLOBAL_LOWTIER_HALF_PERMILLE=500，作用在 k/e 之后的最终权重；3★ 排山倒海不动。全局 9 条权重和 303→155，协议/检查点/MOD_BUILD 37 不动 |
| ab13cbf | 2026-10-10 | 全局降权 docs：07 同日条目（改前→改后表+相对口径）+ 06 §8.7 公式/旋钮 + 03 §5.64 验收 |
| d8c671a | 2026-10-10 | 换一批去重：同屏排除+一屏冷却（用户令「刷掉的 buff 别反复出现」）。RollChoices(bool theReroll)：重抽把本屏现三条从候选摘掉（池子够必出全新三条），增益屏另摘上一屏露过脸没拿的（含被刷掉的那批）；池子不够按保底 min(池子,3) 条自动放宽，不让三选一屏变秃。新记账 mPickSeenBuffs/mPickCooldownBuffs 纯内存、不落检查点；消费（拿走/放弃）生成下一屏冷却，拿走的不进冷却（跨屏叠层照旧）。植物屏只做同屏排除（无跨屏冷却）。旧注释「重抽必然≠刚才三条」与旧实现不符（只换种子、可能重合），本批补上真排除并改注。协议/检查点/MOD_BUILD 37 不动 |
| 3e579c0 | 2026-10-10 | 换一批去重 docs：03 §5.65 批记录+5 条验收；07 同日条目（查因+落法+范围）；06 §8.7 补去重口径 |
| 422791d | 2026-10-10 | 公告更新：Build 37 速览（无尽/高级选项+植物僵尸/第二 buff 批/平衡与修复）——用户令 35-37 三代重点进公告第 1 页，中英双版 |
| 9abc694 | 2026-10-10 | 部署记录补记：Build 37 发布链（Release 541deb72 → 推 1559a53..422791d → runtime 换版冒烟过 → builds 91b6a12）——03 §5.63 填部署记录含公告更新 422791d；§5.64/§5.65 补随链落地注 |
| 5a2d5b4 | 2026-10-10 | 无尽 buff 固定 2 + 名单快速解锁 + 滑动收阳光（2026-10-10 用户三定案） |
| a2fec39 | 2026-10-10 | 无尽收尾三改 docs：03 §5.66 批记录+8 条验收（buff 固定 2/名单虚拟关号 K=2/滑动收阳光）；07 两条数值条目（改前→改后+口径+样例） |
| 6fffd3f | 2026-10-10 | 仙人掌「Tri-Spike」索敌对齐三线射手（用户反馈「拿了 buff 只向下分裂」）：UpdateShooter 仙人掌分支持条时索敌改三行 OR（本行/上行/下行任一有敌就开火，照 LaunchThreepeater 判定与 RowCanHaveZombies 闸门）；FindTargetAndFire 给持条加忧郁菇同款「无目标也放」豁免——本行无怪也空放主弹+侧弹，同三线「任一行有敌→三行都放」口径。未持条维持只查本行逐位不变；侧弹本体（MOTION_THREEPEATER/VelY/ShadowY）此前已与三线逐参数一致，本批不动 |
| d3d1b3e | 2026-10-10 | Tri-Spike 索敌 docs：03 §5.67 批记录+4 条验收；07 同日条目（改前→改后+口径：侧弹本已与三线一致、差异全在索敌门） |
| cb91e9e | 2026-10-10 | 穿透弹命中去重环：贴身僵尸不再被推跳漏打 |
| eb53723 | 2026-10-10 | 三线多发补斜向：相邻行多发弹不再是叠画本行的废弹 |
| e531bbf | 2026-10-10 | 弹体两修 docs：03 §5.68 批记录+6 条验收；07 条目（穿透去重环与三线多发斜向，口径=实弹率修复非数值改动） |
| 004e5ff | 2026-10-10 | 协议 MOD_BUILD 38：START_LEVEL 载荷 24→25（尾部 runBossFlag 开关字节，混版断线保护）+ MSG_SEND_SUN=19 预留；检查点 v12→v13（尾部 U16 boss 位，v12 照读、无尽恒 0） |
| 6f2f96b | 2026-10-10 | 高级选项第五行「巨型 Boss（闯关关底）」关闭/开启：选择经 RunModeDialog → StartRun → SendStartLevel 全链带动队友；AlignRunToHost 按房间级覆写对齐（不参与重建判等），非法值钳 0 |
| 83ecbd3 | 2026-10-10 | 关底巨型 boss：闯关最后一关在第一席棋盘刷巨型红眼巨人（体尺 2.0、血量=原血量×顺位乘数和×关卡基数×2）；全控制免疫收口 CanBeChilled/ApplyButter/StartMindControlled/SwitchLanes，砸击破耐砸（耐砸特例跳过直接压扁）；无尽档恒不刷，漏怪照普通传递接力 |
| f77594f | 2026-10-10 | 发阳光（MOD_BUILD 38）：点阳光银行定向赠 100（税 10、5s 冷却） |
| 75718c7 | 2026-10-10 | docs：关底巨型 boss + 发阳光批（§5.69、MOD_BUILD 38）+ 发阳光目标改环绕口径 |
| 3433c67 | 2026-10-10 | docs：§5.69 补部署记录（runtime af6e9480） |
| 6cf0caf | 2026-10-10 | docs：§5.69 验收③漏怪措辞订正（boss 照普通传递，不排除） |
| d567c3d | 2026-10-10 | 发阳光改 G 键直选 + 收款天降演出（2026-10-10 用户拍板，取代点银行/环下一位版） |
| 2975c76 | 2026-10-10 | docs：发阳光 G 键直选+天降演出追记（§5.69 正文/验收⑤⑥⑦/04 玩法表与版本纪律）；部署 26587a00 |
| e0a5fd7 | 2026-10-10 | docs 门面刷新到 MOD_BUILD 38：README 状态段推进（M0–M4 全链+最新批）、docs/README 现状补第 7 条、03 §零 括注/待办/历史导航更新 |
| a0478bd | 2026-10-10 | 启动公告更新到 Build 38：巨型 Boss + G 键发阳光进【本版更新】，玩法速览键位行补 G |
| a5def1b | 2026-10-10 | docs：新增 08-更新日志.md（完整历史 M0→Build 38，面向玩家）+ 更新日志纪律（规范 §11 / CLAUDE.md / docs 索引） |
| d2a1238 | 2026-10-10 | 仙人掌三向尖刺上发真修复：影子补偿 ±80→同列实际行距（§5.67 误判为索敌，真因=影差 24<28 被收掉） |
| efc3891 | 2026-10-10 | docs：§5.70 三向尖刺上发真修复批次记录（根因数值链+验收五条）+ 08-更新日志 Build 38 段补条目 |
| b9f87ba | 2026-10-10 | docs：§5.70 补部署记录（Debug 813b6201，推送 a5def1b..efc3891，runtime 换版完成） |
| 5363e75 | 2026-10-10 | 卷心菜「重投」伤害 buff 修复：挂点从 DoSplashDamage 挪到 DoImpact 单发分支 |
| 3f6965c | 2026-10-10 | docs：§5.71 卷心菜重投真修复记录（根因=挂点在溅射死路径）+ 三线两 buff 复查 + 同类全表审计（唯一死挂点） |
| 0e34b59 | 2026-10-10 | docs：§5.71 补部署记录（Debug 3d1ad5ec，推送 b9f87ba..3f6965c，runtime 换版完成） |
| 01c09f6 | 2026-10-10 | 矛盾对跨屏互斥：§8.6.1 从「同屏去重」扩为「持有即排除」（用户 2026-10-10 拍板） |
| f79a288 | 2026-10-10 | docs：§8.6.1 扩为跨屏「持有即排除」（用户拍板）+ 03 §5.71 追补与验收⑥ + 08 互斥条目 |
| 33053a2 | 2026-10-11 | 局内消息横幅改「最近三条同屏」：最新在最上、各自计时 5 秒（原只画队首 8 条排队轮播） |
| f46937b | 2026-10-11 | docs：§5.72 消息横幅三窗口批次记录（根因=只画队首）+ 08 Build 38 段 10-11 条目 + 顶栏指针更新 |
| 2fa3d20 | 2026-10-11 | 服务器：重连协议帧 + 席位保留 + REJOIN 受理（MOD_BUILD 39 服务器侧） |
| 422d4f0 | 2026-10-11 | 服务器：重连黄金向量 +3 与 README 行为说明 |
| 3ec7602 | 2026-10-11 | 协议：重连三帧 + 席位保留常量（MOD_BUILD 38→39） |
| 6e24719 | 2026-10-11 | 魅惑菇改组：迷魂香改「血量+攻击都 ×2/层」双轴；Devotion 暂时下架（池排除+效果摘除+图鉴过滤） |
| 4b931d6 | 2026-10-11 | docs：§5.73 魅惑菇改组（迷魂香双轴 + Devotion 下架 + 图鉴池过滤）批次记录与验收 |
| cd7b470 | 2026-10-11 | 联机重连：会话层 RECONNECTING 状态机 + 席位保留配套（MOD_BUILD 39） |
| df04f83 | 2026-10-11 | 卡槽 8→10：闯关与无尽 RUN_SEED_SLOTS 改 10 格（经典战役不动），选卡门口径与注释同步 |
| d857487 | 2026-10-11 | docs：§5.74 卡槽 8→10 批次记录与验收；07 数值日志、08 更新日志、04 联机、强度调整同步 |
| a60a8e0 | 2026-10-11 | 三选一空位闪退修复：植物候选抽干时空位 SEED_NONE 护栏（构造不取名+置灰、绘制跳过）；金标限增益屏 |
| 87e5ba5 | 2026-10-11 | docs：§5.75 三选一空位闪退根因与验收（玩家报障，2026-10-10 两份 log）；08 更新日志条目 |
| a6803d4 | 2026-10-11 | 重连批 D1：掉线重连框落地——OnlineStartDialog 三扩展（取消并退出/SetBody/自有对话框 id）+ LawnApp 挂点与四事件分支；NetSession 重连进度 getter；MOD_BUILD 39 |
| 2be66d4 | 2026-10-11 | 重连批 D2：状态小条——重连最优先行「正在重连……（第 n 次）」+ 名册席位「（掉线中）」（MOD_BUILD 39） |
| 60354d8 | 2026-10-11 | docs：§5.76 掉线自动重连 + 席位保留批次记录与 11 条实机验收（MOD_BUILD 39）；04 掉线行/代次链、05 帧表与版本纪律、08 玩家口径段同步 |
| 53c67ca | 2026-10-11 | 发包前：启动公告第 1 页换 Build 39 速览（重连头条 + 38 的 boss/发阳光 + 卡槽 10 格/闪退修复，中英双语）；03 §5.76 挂账行改已办（2026-10-11 用户令发包） |
| 18409e4 | 2026-10-11 | 公告第 1 页压回 8 行：10 行把框撑到 599x722（屏幕 600，日志实测，多出的一行整档跳段 +108px）——改动与修复两条并一行，恢复 Build 37/38 的 599x614 版式 |
| 0c3c219 | 2026-10-11 | 启动公告两语各压回 8 行：框高=所有页最高行数（实测 8/9/10 行→614/668/722px，54px/行）——CN 页1 G 条并回一行、页2 局内条并回单行（G 已进页 1），EN 页1 同并、页2 由 11 行收短到 8 行（原会撑到 ~776px）；行数约束写进函数头注释 |
| 2877f8d | 2026-10-11 | MOD_BUILD 39 发布记录：§5.76 补推送（f79a288..53c67ca 首推 + 18409e4/0c3c219 二推）/Release exe md5/builds c5ecce8/MOD39 玩家包 md5/中继二进制 sha256 与宝塔部署要点/公告框 8 行=614 线性口径（更正 18409e4 的 +108px 误释） |
| f251ba4 | 2026-10-11 | 关底 boss 批：随机双 boss + 旗波出场 + 血量×3 + 橄榄球只受减速 + 极速啃食 + 地刺王卡死修复 |
| c714bf5 | 2026-10-11 | docs 关底 boss 批：03 §5.77（批次记录+验收 8 条）+ 08 MOD39 段玩家口径 + 07 数值段 + 04 速查行 + 强度总表两行 |
| 5df1aa5 | 2026-10-11 | Devotion 恢复进池（附加「魅惑菇冷却 ×6」负面）+ 毁灭菇伤害词条降 2★ |
| d8841c0 | 2026-10-11 | docs：§5.78 Devotion 恢复（附加冷却 ×6）与毁灭菇降 2★ 批次记录与验收 |
| eed3dff | 2026-10-11 | 词条查看器跟随语言档：三表 96 条补中文名 mNameZh + GetRunChoiceDisplayName 取串口（局内菜单「本局词条」中文档显示中文名；三选一屏按钮/图鉴标题仍英文，docs/03 §5.79） |
| c2379d9 | 2026-10-11 | docs 词条查看器语言批：03 §5.79（批次记录 + 96 条中文名全表带新拟标注 + 验收 6 条）+ 08 MOD39 段玩家口径 |
| 29c5ae3 | 2026-10-11 | docs：§5.79 补代码提交哈希 eed3dff |
| 5d8d68f | 2026-10-11 | 无尽开局改动：卡池开局即满 48 株（FillPoolWithAllPlants，含读档补齐）+ 开局增益 2→4 屏；植物屏经 CanOfferPlantPick 自动归零、每关改走选卡界面挑 10 株（docs/03 §5.80） |
| 29d5aa1 | 2026-10-11 | docs：无尽开局改动三份（03 §5.80 批次记录+验收 6 条 / 07 数值日志 / 08 玩家口径 MOD39 段条目） |
| 28a0f27 | 2026-10-11 | 忧郁菇「耐砸」重调：血量 1000→500、每击固定 200→50（硬吃 5→10 次）；落点=词条文案/注释 + Plant 绝对值覆盖 + Zombie 砸击两分支与啃咬补差（docs/03 §5.81） |
| 981aede | 2026-10-11 | docs：耐砸调整四份（03 §5.81 批次记录+验收 4 条 / 06 表行 500/50 / 07 数值日志 / 08 玩家口径 MOD39 段条目） |
| acd0cc7 | 2026-10-11 | 铁门植物头 ×5（带防具的植物僵尸）：五种植物头各挂铁门盾 1100（枚举 37-41）；投放走植物僵尸开关 OR 行 + 权重 250 + 生存随机池闸 + 图鉴/初始化门臂层；MOD_BUILD 39→40（漏怪帧 mZombieType 会带出新值）（docs/03 §5.82） |
| fb52fb1 | 2026-10-11 | docs 铁门植物头批：03 §5.82（批次记录 + 验收 7 条 + fake smoke 断言对齐注）+ 07 数值日志（点数 5/8/8/8/8、权重 250 表）+ 08 MOD_BUILD 40 段（玩家口径）+ 04 §四 39→40 代次链与当前值注（=38→=40 顺带校正） |
| 414a46d | 2026-10-11 | 急袭上限 4 层（用户令「攻速buff上限改为四层」）：全局表 mMaxStacks ∞→4（×0.75 叠乘满层 ≈ ×3.16 攻速）；单株攻速条一律不动；文案「至多 4 层」经 GetRunChoiceDesc 机械追加；旧档超限保留不追溯（docs/03 §5.83） |
| 157c7b4 | 2026-10-11 | docs 急袭上限批：03 §5.83（批次记录 + 验收 3 条 + MOD_BUILD 不动的口径）+ 07 数值日志（∞→4 层表）+ 08 MOD_BUILD 40 段条目（玩家口径）+ 06 §三定案表行与 §8.2.1 现况表行两注 |
| fef7492 | 2026-10-12 | 无尽难度乘数改为 min(1.1^(n-1), (1+ln n)²)（用户 2026-10-12 令） |
| 7d81bdc | 2026-10-12 | 完整档（25 关）每关增益屏收紧为固定 1（用户 2026-10-12 令） |
| 5c496ab | 2026-10-12 | 关底 boss 改出一号位（固定席位 1）而非房主席位（用户 2026-10-12 令） |
| 1793648 | 2026-10-12 | docs 无尽曲线批：03 §5.84 + 04 五处 + 07 两条目 + 08 MOD_BUILD 41 段 + 强度调整.md 奖励表 |
| 5b5119c | 2026-10-12 | 无尽档关数显示：DrawLevel 画「第 n 关」取代原型关号 |
| b642910 | 2026-10-12 | docs：无尽关数显示批次记录（03 §5.85 + 08 MOD_BUILD 41 段条目） |
| f6db273 | 2026-10-12 | 无尽名单封顶改雾场景末格：跨屋顶剔除名单不再丢怪（MOD_BUILD 42） |
| fa4e3ea | 2026-10-12 | docs：无尽名单封顶 19 修法（03 §5.86 + 08 MOD_BUILD 42 段 + 04/07） |
| b0f7362 | 2026-10-13 | Release 构建加符号：/Zi + /DEBUG /MAP 产 map/pdb（崩溃取证管线铺垫） |
| bfe7d40 | 2026-10-13 | 全屏修复批①：DDraw 重建后回读呈现矩形 + 全屏禁黑边抓区 |
| f5eaafd | 2026-10-13 | TodDebug：修日志文件打不开就静默秒退（fwrite NULL）；加最近 200 条日志环形缓冲 |
| 2185b39 | 2026-10-13 | SEHCatcher：崩溃产物落 exe 目录 + crash.dmp 全线程转储 + map 搜索序 |
| c321671 | 2026-10-13 | 崩溃现场带游戏态：LawnApp::GetGameSEHInfo + 日志尾 + 中文提示 + 启动环境探针 |
| 936b5c7 | 2026-10-13 | NetLink 卡顿探针：慢发送（>250ms）与接收队列深度（>=192/256）记账 |
| 74ee878 | 2026-10-13 | 全屏修复批②：全屏改无边框窗口 + 解开禁用 + 独占结构性封死 |
| 110de14 | 2026-10-13 | docs：闪退取证升级批（§5.87）——本机两测记录 + 玩家口径更新日志 |
| 86e6f00 | 2026-10-13 | 全屏修复批③：窗口模式切换改原地换装（不重建窗口）+ 显示变更贴合 + 激活重钉置顶 |
| 6377a18 | 2026-10-13 | 全屏修复批④：文档——规范 §7/§3.1 改写 + 03 §5.88 全屏修复批 + 08 玩家条目 + 01 自检项 |
| 7a756a2 | 2026-10-13 | 汉化批⓪：pak 覆盖基础设施——PakInterface 末赢索引 + LawnApp 中文模式挂 pvz_zh.pak |
| 43561bc | 2026-10-13 | 汉化批②A：引擎 UTF-8 字型升级——全链按字解码 |
| a19da93 | 2026-10-13 | 汉化批②C：中文模式叠载 LawnStrings_zh（缺键保留英文、失败静默回退） |
| 24ccc5f | 2026-10-13 | 汉化批②C修复：注册表链断改 HKCU 直通 + 中文字节喂 ctype 的两处断言 |
| 02cb9c1 | 2026-10-13 | 汉化批②C修复：断行器 aCharStart 语义错乱（红 KEYWORD 叠印根因） |
| 5c6904a | 2026-10-13 | 联机发阳光：收方横幅带发送者名字（XXX 送你阳光） |
| 091c7be | 2026-10-13 | docs：§5.89 收阳光横幅带名字（MOD_BUILD 42 不变）+ 08 玩家条目 |
| b0ed855 | 2026-10-13 | 高级选项：增益倍选 ×1/×2/×3（MOD_BUILD 43）+ 面板六行分两页翻页 |
| 02a1e30 | 2026-10-13 | docs：§5.90 增益倍选 + 面板翻页（MOD_BUILD 43）+ 04 代次链 + 08 玩家条目 |
| 3ef2936 | 2026-10-13 | 高级选项面板：实机遇出格修复——CalcSize 取整钳到屏高内（614→560）+ §5.90 探针实测补记 |
| 52fba67 | 2026-10-13 | 汉化批④：mod 自绘残留英文清理（9 处走 Tr 双语，切换即生效）+ 语言选项「重启后生效」提示 |
| 51bc30d | 2026-10-13 | 联机开局修复：START_LEVEL_PAYLOAD_SIZE 漏改 25→26（nettest 35 项全红→全绿） |
| 1c1cc0f | 2026-10-13 | docs：§5.91 中文汉化批（⓪②①④全链 + 巡屏证据 image15-23 + nettest 拦截记录）+ 08 玩家条目 |
| bbfde73 | 2026-10-13 | docs：§5.91 发布记录（推送 2877f8d..1c1cc0f 46 笔 / builds 695c93b / MOD43 包定稿 17a3bf87 / 符号归档与 map 同步） |
| 5b0826f | 2026-10-13 | 花园批①：商店与禅境花园入口恢复（商店只摆花园两页） |
| 0d1d6b6 | 2026-10-13 | 花园批②：三选一金币重摇（MOD_BUILD 44） |
| 109c42e | 2026-10-13 | docs：§5.92 花园批（花园/商店回归 + 三选一重摇，MOD_BUILD 44）+ 07 重摇定价 + 08 玩家条目 |
| 8c30527 | 2026-10-13 | 花园批③：三选一「重摇」改阶梯价（同日用户令「每次*10，从100开始」，拍板每屏重置） |
| 61c37d6 | 2026-10-13 | docs：§5.92 重摇阶梯价修订（×10/次、每屏重置）+ 07 表 + 08 玩家条目 |
| 52d8ed7 | 2026-10-14 | 辣椒僵尸批：修「死后一直烧」——爆一次即摘头（2026-10-14 玩家报） |
| a5030d0 | 2026-10-14 | 三选一批：钱罐显示同口径 + 重摇恒付费 + 花盆荷叶出率×5 + 说明行缺字/叠字修复 |
| b9535a6 | 2026-10-14 | docs：§5.93 三选一批（金币统一/恒付费/辣椒僵尸/×5/缺字叠字）+ 07 表 + 08 玩家条目 |
| 6057cf2 | 2026-10-14 | 阶梯+僵尸强化批 C1：数据面（阶梯五档表 + 僵尸强化 8 条层数 + 存档 v14） |
| d4f333d | 2026-10-14 | 阶梯批 C2：高级选项两行（场景难度五档 + 僵尸 buff 0/1/2）+ 全链线索 |
| 3a6fd0b | 2026-10-14 | 阶梯批 C3：START_LEVEL 26→36（+阶梯档/buff 档/8 条层数快照）+ MOD_BUILD 45 + nettest |
| a8f7efd | 2026-10-14 | 阶梯批 C4：八条僵尸强化效果钩子（迅捷/铁壁/护心/狂食/再生/免控/墓碑者/无视阻挡） |
| bc6d973 | 2026-10-14 | 三选一屏按钮文字出框根因修复：石头贴片铺满控件宽（绘制层） |
| ac300f5 | 2026-10-14 | docs：§5.94 三选一按钮出框根因修复记录 + 更新日志条目（MOD_BUILD 45 不变） |
| 6361750 | 2026-10-14 | 阶梯批 C5：僵尸强化投票三消息 20/21/22（开票/投票/结果）全链——协议编解码 + 会话层收发 + 队列清理 |
| 0e08297 | 2026-10-14 | 阶梯批 C5b：投票候选取数 4→3（三选一口径；弃权=choice 3、开票载荷 6 字节） |
| ab341ef | 2026-10-14 | 中文断行/整块绘制五件套从 RunPickDialog 提取进 ModText：断单元、「不能起行」两表 + WrapLines / WrapLineCount / DrawWrappedCjk |
| bef727a | 2026-10-14 | 阶梯批 C6：僵尸强化投票框 + 全流程接线（触发/计票/超时/单人） |
| 13cc483 | 2026-10-14 | 阶梯批修：投票字段构造归零——进关卡死后卡主菜单的根因 |
| 6caca01 | 2026-10-14 | 三选一重摇底价二次定案：10 币（按钮显示 $100 起）——修「从1000开始、有500多币也点不动」 |
| a52f757 | 2026-10-14 | 本地场景进路绕过联机开局守卫——修「点休闲花园立刻被弹回菜单」 |
| 55c0966 | 2026-10-14 | 冒烟钩子（临时，冒烟完删）：-tod 下 'L' 建监听会话，复现花园弹回现场 |
| 0252614 | 2026-10-14 | docs 同批：§5.95 花园弹回+重摇二次定案、07 底价改 10 币条目、08 MOD_BUILD 45 玩家口径 |
| a451519 | 2026-10-14 | 冒烟钩子退场：删 Board::KeyChar 的 'v'（僵尸投票）与 'L'（监听会话）两个 TEMP 钩子 |
| 0536f48 | 2026-10-14 | §5.95 本机验证段改实测实况：真 UI 路径复现（开房→花园），键盘钩子对合成按键无效结论 |
| 81f4813 | 2026-10-14 | fix: reset votes at run boundaries to prevent stale modal/state; verified Debug incremental build |
| 0893f85 | 2026-10-14 | 交接文档：MOD45 批收尾 + 花园弹回/重摇两修复状态快照（供新会话续接） |
| 3ccc9ec | 2026-10-14 | fix: widen vote round IDs and reject invalid result bounds; verified Debug build, codec probe and T51 |
| d9d379a | 2026-10-14 | fix: widen and saturate run health math to prevent negative HP; verified Debug build and high-value helper probe |
| 8fbecc0 | 2026-10-14 | fix: resolve votes by current seat membership rather than cumulative count; verified Debug build and mask scenario review |
| 7717f97 | 2026-10-14 | fix: relay boss identity and HP limits across all seats; verified Debug build, codec and targeted TCP/relay tests |
| 8bfc67e | 2026-10-14 | docs: consolidate build 46 fixes and record Release and targeted validation results |
| c5ccdbe | 2026-10-14 | 验收跳板 A+B：'v' 开强化票 / 'M' 加万币钩子（-tod 门禁内）+ mk_run_checkpoint.pl 检查点注入器（v13 dump 兼容、布局逐字段核对 Save、回环自测过）+ docs/03 §5.97 |
| d4d17b8 | 2026-10-14 | 实机反馈五修批（MOD_BUILD 47）：舞王每波1只+波构成日志 / M·v键大小写 / 弃权随机一条 / 护心改首伤一次性 / 词条屏加僵尸强化段 |
| cc85a3a | 2026-10-15 | build: fix MSVC header dependencies and offline vendor builds; validated clean Debug and Release |
| b5cfa3a | 2026-10-15 | feat: implement 15 standalone plant buffs and voucher persistence; preserve IDs, verified x86 builds and 20 targeted checks |
| b7de9bd | 2026-10-15 | feat: make three-star fume buff spray continuously at five hits per second |
| a25fcdc | 2026-10-15 | feat: add PC plant/shovel hotkeys and move health keys; verified x86 Release and input checks |
| 1cd6a51 | 2026-10-15 | feat: add two-star Scaredy-shroom percent damage buff capped at 800 |
| 75d737f | 2026-10-15 | 快捷短语表 8→9 条（MOD_BUILD 51）：9 条新文案、表情 id 后移 10..17；T/E 面板键位与行数按页取（顺修表情页越界读 EMOTE_SEEDS 潜伏 bug） |
| 1d1403e | 2026-10-15 | docs：快捷短语 9 条批记录（03 §5.102 / 08 MOD_BUILD 51 / 04 §四 51 代次） |
| 8d5466c | 2026-10-15 | 快捷短语英文两处审定：⑤区区(可自指)→?!Maggot?!、⑧蹦蹦(蹦极僵尸)→Bungee! Bungee! |
| 0a46b1a | 2026-10-15 | 忧郁菇 emotional damage 新词条 + 耐砸改版（不抬代）：孤高（8 格无植物）伤害 ×3、命中回 2% 最大生命；耐砸 1000 血/仅巨人砸 -200 坚果口径、删每击固定扣 50 补差，1★→2★；注入器 --buffs id 上限 110 |
| 2da2cd0 | 2026-10-15 | 坚果保龄球新词条（不抬代）：费用变 0、冷却 15 秒、种下的坚果变保龄球滚撞（IsWallnutBowlingLevel 关卡判定不动，6 行为点按词条放行）；与耐砸/荆棘之壁三角互斥；注入器 --buffs id 上限 111；docs 03 §5.104/06 §8.6.1/07/08 同批 |
| c79c1b3 | 2026-10-15 | 补齐接力标记声明，配合已入库落地与绘制调用；PC增量构建通过 |
| 22eeb96 | 2026-10-15 | 记录四项协作方案与双端阶段交接，区分已编译及未实现范围；文档链接与diff检查通过 |
| e69c6cf | 2026-10-15 | 全局词条削弱定案：火力每层 +30%→+20%（∞ 不变）；其余全局条维持现值、不加层上限（用户拍板否掉 cap 方案）；runtime 待下次干净部署生效（树上另有接力批 WIP 勿编入） |
| b3c534d | 2026-10-15 | Build52实现关间借用与阳光求援，隔离临时词条并增加回执去重；PC编译与21项定点检查通过 |
| d30f30a | 2026-10-15 | 收口Build52求援去重与阶段提示，补移动端借用到账定点证据；双端编译通过 |
| dd140f4 | 2026-10-15 | 协作批 UI 五项（不抬代 Build52）：接力入场来源横幅（同源同行 2.5s 合并）、观战侧接力金箭头（ZFLAG 0x08）、协作页借用来源与生效层数展示、大厅换位选人框（任选队友+倍率/顺位阳光/末席预览+5s 超时）、局内协作字牌去石框；协议字节零改动；docs 03 §5.105/08/09 同批 |
| 946f690 | 2026-10-15 | docs: record user buff decisions and design guidance for agents |
| ae489ee | 2026-10-15 | fix: open garden economy without campaign completion; isolate local scenes; verify Release and 11 probes |
| 8a0ac05 | 2026-10-15 | Improve room status and six-seat relay overview |
| 1ae3ec3 | 2026-10-15 | Keep relay hints accurate while seats are being assigned |
| 5224048 | 2026-10-16 | 修复接力箭头被动画覆盖并保留读档标记；双端编译与真实渲染及旧档定点通过 |

## Android：独立移植基线至 ed8fb76 功能审计

共31笔。移动仓库始建晚于PC37，故列其全部已入分支历史，避免以PC代次漏掉触摸/资源/构建改动。

| 提交 | 日期 | 原始说明 |
|---|---|---|
| ac3e1e5 | 2026-10-13 | Android baseline: pin Portable snapshot, add isolated dev package and build handoff; verified ARM64 build and MuMu Android 15 startup |
| 1859da4 | 2026-10-13 | Android seed drag: plant on release and cancel invalid drops; validated sunflower on MuMu Android 15 |
| eee328c | 2026-10-13 | docs: move Android collaboration to dedicated repository and update clone instructions |
| dda1580 | 2026-10-13 | Audit port identifiers: plant IDs match, eight armored zombie behaviors remain; verified fixed snapshots with enum parser |
| acdad0a | 2026-10-13 | feat: add run prototype and Chinese language support |
| 4cefbd8 | 2026-10-13 | docs: plan five-scene run rules and focused verification |
| 38dfa89 | 2026-10-13 | Port finite solo run battle rules and preserve retry checkpoints |
| 6640821 | 2026-10-13 | Plan staged economy, attack rhythm and damage buff port |
| df08a1b | 2026-10-14 | Plan complete PC buff port against build 44 for cross-platform parity |
| 8ac5ce5 | 2026-10-14 | Port complete PC Build 44 run buffs and plant dependencies |
| 23f0748 | 2026-10-14 | Port armored zombies and advanced solo rules; prepare PC version synchronization |
| e69f74b | 2026-10-14 | Synchronize pinned PC Build 45 rules, checkpoints and vote protocol |
| 41ab83f | 2026-10-14 | Connect native lobby and pinned Build 45 multiplayer game flow |
| ac3a11e | 2026-10-14 | Match PC lobby flow and select relay port automatically |
| 062f6d6 | 2026-10-14 | Plan PC-compatible mobile chat, emotes and sun gifts |
| ad7a541 | 2026-10-14 | Add mobile cooperation controls and synchronize pinned PC Build 46 |
| d658760 | 2026-10-14 | docs: plan seed bank and shovel keyboard controls |
| 171dbcf | 2026-10-15 | feat(android): add seed bank and shovel keyboard controls |
| 1ea2a20 | 2026-10-15 | 同步PC Build52并补移动观战和协作入口，完成借用求援及临时词条；编译签名与定点到账检查通过 |
| 7492449 | 2026-10-15 | 记录Build52移动UI视觉探查与分批改法，避免接手误用旧清单；复核5图及4点对比度 |
| 3ab8ae7 | 2026-10-15 | 记录UI计划已传递及实施边界，保证截断后可接续 |
| a7b8208 | 2026-10-15 | docs: checkpoint current UI handoff and PC alignment scope |
| dacd49b | 2026-10-15 | port PC cooperation status and seat-swap changes from inherited work |
| 49b683a | 2026-10-15 | align Android run and co-op interfaces with PC game UI |
| a8b00c9 | 2026-10-15 | docs: audit shared features blocked by adventure progression and plan fixes |
| 3d5089a | 2026-10-15 | fix: align shared features and garden economy with PC Build53; verify native and emulator transactions |
| 7ba1598 | 2026-10-15 | Align room status and touch seat overview with PC |
| 1b74abd | 2026-10-16 | Sync global Firepower nerf from PC e69c6cf: +30%/layer -> +20%/layer |
| db22f9b | 2026-10-16 | 修复接力怪标记遮挡并以可选存档块保留，固定PC 5224048；Native与PC渲染读档定点通过 |
| 2596730 | 2026-10-16 | 记录接力标记最终候选与定点边界，明确不覆盖当前对局 |
| ed8fb76 | 2026-10-16 | Document functional parity gaps and runtime evidence; prioritize exit, input and modifier fixes |
