# 植物/僵尸编号兼容审查

固定输入：Windows 7a756a2（docs/reference/windows），Portable 141c1476（src/ConstEnums.h）。以下表由 tools/audit/enum-audit.ps1 解析显式整数与隐式递增得到；遇到其他表达式会失败，不猜值。

## SeedType

| 名称 | PC | Portable | 结果 |
|---|---:|---:|---|
| SEED_PEASHOOTER | 0 | 0 | 一致 |
| SEED_SUNFLOWER | 1 | 1 | 一致 |
| SEED_CHERRYBOMB | 2 | 2 | 一致 |
| SEED_WALLNUT | 3 | 3 | 一致 |
| SEED_POTATOMINE | 4 | 4 | 一致 |
| SEED_SNOWPEA | 5 | 5 | 一致 |
| SEED_CHOMPER | 6 | 6 | 一致 |
| SEED_REPEATER | 7 | 7 | 一致 |
| SEED_PUFFSHROOM | 8 | 8 | 一致 |
| SEED_SUNSHROOM | 9 | 9 | 一致 |
| SEED_FUMESHROOM | 10 | 10 | 一致 |
| SEED_GRAVEBUSTER | 11 | 11 | 一致 |
| SEED_HYPNOSHROOM | 12 | 12 | 一致 |
| SEED_SCAREDYSHROOM | 13 | 13 | 一致 |
| SEED_ICESHROOM | 14 | 14 | 一致 |
| SEED_DOOMSHROOM | 15 | 15 | 一致 |
| SEED_LILYPAD | 16 | 16 | 一致 |
| SEED_SQUASH | 17 | 17 | 一致 |
| SEED_THREEPEATER | 18 | 18 | 一致 |
| SEED_TANGLEKELP | 19 | 19 | 一致 |
| SEED_JALAPENO | 20 | 20 | 一致 |
| SEED_SPIKEWEED | 21 | 21 | 一致 |
| SEED_TORCHWOOD | 22 | 22 | 一致 |
| SEED_TALLNUT | 23 | 23 | 一致 |
| SEED_SEASHROOM | 24 | 24 | 一致 |
| SEED_PLANTERN | 25 | 25 | 一致 |
| SEED_CACTUS | 26 | 26 | 一致 |
| SEED_BLOVER | 27 | 27 | 一致 |
| SEED_SPLITPEA | 28 | 28 | 一致 |
| SEED_STARFRUIT | 29 | 29 | 一致 |
| SEED_PUMPKINSHELL | 30 | 30 | 一致 |
| SEED_MAGNETSHROOM | 31 | 31 | 一致 |
| SEED_CABBAGEPULT | 32 | 32 | 一致 |
| SEED_FLOWERPOT | 33 | 33 | 一致 |
| SEED_KERNELPULT | 34 | 34 | 一致 |
| SEED_INSTANT_COFFEE | 35 | 35 | 一致 |
| SEED_GARLIC | 36 | 36 | 一致 |
| SEED_UMBRELLA | 37 | 37 | 一致 |
| SEED_MARIGOLD | 38 | 38 | 一致 |
| SEED_MELONPULT | 39 | 39 | 一致 |
| SEED_GATLINGPEA | 40 | 40 | 一致 |
| SEED_TWINSUNFLOWER | 41 | 41 | 一致 |
| SEED_GLOOMSHROOM | 42 | 42 | 一致 |
| SEED_CATTAIL | 43 | 43 | 一致 |
| SEED_WINTERMELON | 44 | 44 | 一致 |
| SEED_GOLD_MAGNET | 45 | 45 | 一致 |
| SEED_SPIKEROCK | 46 | 46 | 一致 |
| SEED_COBCANNON | 47 | 47 | 一致 |
| SEED_IMITATER | 48 | 48 | 一致 |
| SEED_EXPLODE_O_NUT | 49 | 49 | 一致 |
| SEED_GIANT_WALLNUT | 50 | 50 | 一致 |
| SEED_SPROUT | 51 | 51 | 一致 |
| SEED_LEFTPEATER | 52 | 52 | 一致 |
| NUM_SEED_TYPES | 53 | 53 | 一致 |
| SEED_BEGHOULED_BUTTON_SHUFFLE | 54 | 54 | 一致 |
| SEED_BEGHOULED_BUTTON_CRATER | 55 | 55 | 一致 |
| SEED_SLOT_MACHINE_SUN | 56 | 56 | 一致 |
| SEED_SLOT_MACHINE_DIAMOND | 57 | 57 | 一致 |
| SEED_ZOMBIQUARIUM_SNORKLE | 58 | 58 | 一致 |
| SEED_ZOMBIQUARIUM_TROPHY | 59 | 59 | 一致 |
| SEED_ZOMBIE_NORMAL | 60 | 60 | 一致 |
| SEED_ZOMBIE_TRAFFIC_CONE | 61 | 61 | 一致 |
| SEED_ZOMBIE_POLEVAULTER | 62 | 62 | 一致 |
| SEED_ZOMBIE_PAIL | 63 | 63 | 一致 |
| SEED_ZOMBIE_LADDER | 64 | 64 | 一致 |
| SEED_ZOMBIE_DIGGER | 65 | 65 | 一致 |
| SEED_ZOMBIE_BUNGEE | 66 | 66 | 一致 |
| SEED_ZOMBIE_FOOTBALL | 67 | 67 | 一致 |
| SEED_ZOMBIE_BALLOON | 68 | 68 | 一致 |
| SEED_ZOMBIE_SCREEN_DOOR | 69 | 69 | 一致 |
| SEED_ZOMBONI | 70 | 70 | 一致 |
| SEED_ZOMBIE_POGO | 71 | 71 | 一致 |
| SEED_ZOMBIE_DANCER | 72 | 72 | 一致 |
| SEED_ZOMBIE_GARGANTUAR | 73 | 73 | 一致 |
| SEED_ZOMBIE_IMP | 74 | 74 | 一致 |
| NUM_SEEDS_IN_CHOOSER | 49 | 49 | 一致 |
| SEED_NONE | -1 | -1 | 一致 |

## ZombieType

| 名称 | PC | Portable | 结果 |
|---|---:|---:|---|
| ZOMBIE_INVALID | -1 | -1 | 一致 |
| ZOMBIE_NORMAL | 0 | 0 | 一致 |
| ZOMBIE_FLAG | 1 | 1 | 一致 |
| ZOMBIE_TRAFFIC_CONE | 2 | 2 | 一致 |
| ZOMBIE_POLEVAULTER | 3 | 3 | 一致 |
| ZOMBIE_PAIL | 4 | 4 | 一致 |
| ZOMBIE_NEWSPAPER | 5 | 5 | 一致 |
| ZOMBIE_DOOR | 6 | 6 | 一致 |
| ZOMBIE_FOOTBALL | 7 | 7 | 一致 |
| ZOMBIE_DANCER | 8 | 8 | 一致 |
| ZOMBIE_BACKUP_DANCER | 9 | 9 | 一致 |
| ZOMBIE_DUCKY_TUBE | 10 | 10 | 一致 |
| ZOMBIE_SNORKEL | 11 | 11 | 一致 |
| ZOMBIE_ZAMBONI | 12 | 12 | 一致 |
| ZOMBIE_BOBSLED | 13 | 13 | 一致 |
| ZOMBIE_DOLPHIN_RIDER | 14 | 14 | 一致 |
| ZOMBIE_JACK_IN_THE_BOX | 15 | 15 | 一致 |
| ZOMBIE_BALLOON | 16 | 16 | 一致 |
| ZOMBIE_DIGGER | 17 | 17 | 一致 |
| ZOMBIE_POGO | 18 | 18 | 一致 |
| ZOMBIE_YETI | 19 | 19 | 一致 |
| ZOMBIE_BUNGEE | 20 | 20 | 一致 |
| ZOMBIE_LADDER | 21 | 21 | 一致 |
| ZOMBIE_CATAPULT | 22 | 22 | 一致 |
| ZOMBIE_GARGANTUAR | 23 | 23 | 一致 |
| ZOMBIE_IMP | 24 | 24 | 一致 |
| ZOMBIE_BOSS | 25 | 25 | 一致 |
| ZOMBIE_PEA_HEAD | 26 | 26 | 一致 |
| ZOMBIE_WALLNUT_HEAD | 27 | 27 | 一致 |
| ZOMBIE_JALAPENO_HEAD | 28 | 28 | 一致 |
| ZOMBIE_GATLING_HEAD | 29 | 29 | 一致 |
| ZOMBIE_SQUASH_HEAD | 30 | 30 | 一致 |
| ZOMBIE_TALLNUT_HEAD | 31 | 31 | 一致 |
| ZOMBIE_REDEYE_GARGANTUAR | 32 | 32 | 一致 |
| ZOMBIE_ZOMBATAR | 33 | — | Portable 尚缺 |
| ZOMBIE_PAIL_DOOR | 34 | — | Portable 尚缺 |
| ZOMBIE_CONE_NEWSPAPER | 35 | — | Portable 尚缺 |
| ZOMBIE_PAIL_NEWSPAPER | 36 | — | Portable 尚缺 |
| ZOMBIE_PEA_HEAD_DOOR | 37 | — | Portable 尚缺 |
| ZOMBIE_WALLNUT_HEAD_DOOR | 38 | — | Portable 尚缺 |
| ZOMBIE_JALAPENO_HEAD_DOOR | 39 | — | Portable 尚缺 |
| ZOMBIE_GATLING_HEAD_DOOR | 40 | — | Portable 尚缺 |
| ZOMBIE_SQUASH_HEAD_DOOR | 41 | — | Portable 尚缺 |
| NUM_ZOMBIE_TYPES | 42 | 33 | 不同 |
| ZOMBIE_CACHED_POLEVAULTER_WITH_POLE | 43 | 34 | 不同 |
| NUM_CACHED_ZOMBIE_TYPES | 44 | 35 | 不同 |

## 迁移结论

- 两套 SeedType 的名称和编号全部一致，可以直接复用卡池/检查点编号。
- 共同僵尸编号 0–33 一致；PC 新增 34–41 八种防具变体。Portable 的 NUM_ZOMBIE_TYPES 与缓存专用编号和 PC 不同，不能直接把这些计数/缓存值当线传实体类型。
- NetProtocol 的 EscapedZombie.mZombieType 使用 U16 小端编解码（参考头 487/928/943 行）；解码器只读数值，类型支持必须由会话/战斗接收路径验证。
- 新增僵尸需要同时迁入 Zombie 定义、初始化/防具行为、名称与缓存边界；只加枚举不能宣称类型已支持。在迁入之前不编译/启用包含这些类型的 RunZombieRoster。
- 本轮先迁入 RunState 和 RunBuffs 数据。它们依赖共同植物编号，不需要为了编译先修改僵尸枚举。
