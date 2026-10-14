#!/usr/bin/perl
# mk_run_checkpoint.pl — 闯关检查点 run%d.dat 注入器（验收跳板 B「状态直给」，2026-10-14）
#
# 用途：造一份任意状态的检查点（高关号 / 指定 buff / 僵尸强化层数 / 阶梯档 …），
#       放到 userdata 下，游戏主菜单点「继续」直接落到目标状态——不用人肉打上去。
#       字节格式 = Lawn/Run/RunState.cpp RunState::Save 的 v15 布局复刻（小端，
#       AppendU16/AppendI32 见 RunState.cpp:60-72）；--dump 可回读任意 run%d.dat 对账。
#
# 用法：
#   perl tools/mk_run_checkpoint.pl --dump <run%d.dat>                # 回读打印字段
#   perl tools/mk_run_checkpoint.pl --out <run%d.dat> [选项...]       # 造档
# 选项（默认值见 %OPT）：
#   --mode 0..3       完整/普通/快速/无尽          --diff 0..2   出怪难度档
#   --level N         关序号（0 起，Load 按 LevelCountForMode 封口）
#   --seed N          局种子                       --scene 0..4  无尽锁定场景
#   --pool 0,1,...    卡池（SeedType id；默认 0,1 = 豌豆+向日葵 = StartNew 起始两株）
#   --vouchers 0..2 --awardedlevel N   重摇券及已结算关序号（默认0/-1）
#   --buffs id:count,...   本局 buff 叠层（id 上限 108 = 全局9 + 单株48 + 扩展表52）
#   --ladder 0..4     场景难度阶梯档               --zombiebuff 0..2  僵尸 buff 每场景档
#   --stacks id:count,...  僵尸强化 8 条池的层数（0..7，各 0..255）
#   --votedlevel N    上一票投在第几关（默认 0xFFFF = 没投过）
#   --scale 0..3 --tempo 0..2 --zombotany 0/1 --buffmult 0..2 --boss 0/1  高级选项位包
#
# 例：
#   # 重摇验收：快速档第 4 关（index 3），带 2 层铁壁
#   perl mk_run_checkpoint.pl --out run1.dat --mode 2 --level 3 --buffs 1:2
#   # 无尽票验收：夜场景第 14 关，每场景 2 条、迅捷 3 层
#   perl mk_run_checkpoint.pl --out run1.dat --mode 3 --scene 1 --level 13 --zombiebuff 2 --stacks 0:3
#
# 落点：userdata 目录（GetAppDataFolder()+"userdata/run%d.dat"）。写入/覆盖前先备份
# 原档；游戏必须在关档状态下替换（运行中的游戏退出时会按内存态重写检查点）。

use strict;
use warnings;

my %OPT = (
	mode => 0, diff => 1, level => 0, seed => 20261014, scene => 0, boss => 0,
	ladder => 0, zombiebuff => 0, votedlevel => 0xFFFF,
	scale => 1, tempo => 1, zombotany => 0, buffmult => 0,
	pool => "0,1", buffs => "", stacks => "", vouchers => 0, awardedlevel => -1,
);
my ($dump, $out);
for (my $i = 0; $i < @ARGV; $i++) {
	if    ($ARGV[$i] eq '--dump') { $dump = $ARGV[++$i]; }
	elsif ($ARGV[$i] eq '--out')  { $out  = $ARGV[++$i]; }
	elsif ($ARGV[$i] =~ /^--([a-z]+)$/) {
		defined $OPT{$1} or die "未知选项 --$1";
		$OPT{$1} = $ARGV[++$i];
	}
	else { die "不认识的参数：$ARGV[$i]"; }
}

sub LEVEL_COUNT_FOR { return ( 25, 10, 5, 60000 )[ $_[0] ] }    # 0 完整 1 普通 2 快速 3 无尽

# ── Load 的同款边界校验（RunState.cpp:1062 起）────────────────────────────
$OPT{mode}  >= 0 && $OPT{mode}  <= 3 or die "mode 越界（0..3）";
$OPT{diff}  >= 0 && $OPT{diff}  <= 2 or die "diff 越界（0..2）";
$OPT{level} >= 0 && $OPT{level} <= LEVEL_COUNT_FOR($OPT{mode}) or die "level 越界（0.." . LEVEL_COUNT_FOR($OPT{mode}) . "）";
$OPT{scene} >= 0 && $OPT{scene} <= 4 or die "scene 越界（0..4）";
$OPT{ladder} >= 0 && $OPT{ladder} <= 4 or die "ladder 越界（0..4）";
$OPT{zombiebuff} >= 0 && $OPT{zombiebuff} <= 2 or die "zombiebuff 越界（0..2）";

sub parse_pairs {
	my ($s, $idmax, $cntmax) = @_;
	return () unless length $s;
	my @r;
	for my $p (split /,/, $s) {
		my ($id, $cnt) = $p =~ /^\s*(\d+)\s*:\s*(\d+)\s*$/ or die "格式错：\"$p\"（要 id:count）";
		$id <= $idmax or die "id $id 越界（0..$idmax）";
		$cnt <= $cntmax or die "count $cnt 越界（0..$cntmax）";
		push @r, [ $id, $cnt ];
	}
	return @r;
}

my @pool;
for my $s (split /,/, $OPT{pool}) {
	$s =~ /^\s*\d+\s*$/ or die "pool 格式错：\"$s\"";
	$s < 48 or die "pool id $s 越界（常规植物 0..47）";
	push @pool, $s + 0;
}
@pool <= 48 or die "pool 超 48 株";
my @buffs  = parse_pairs($OPT{buffs}, 108, 255);
@buffs <= 109 or die "buff 条目超 109";
my @stacks = parse_pairs($OPT{stacks}, 7, 255);

# ── --dump：按 v15 布局回读打印 ──────────────────────────────────────────
if (defined $dump) {
	open my $fh, '<:raw', $dump or die "打不开 $dump: $!";
	local $/; my $d = <$fh>; close $fh;
	my $o = 0;
	sub rd { my ($n) = @_; $o + $n <= length($d) or die "文件在 offset $o 处截断"; my $v = substr($d, $o, $n); $o += $n; return $v }
	sub u16 { unpack('v', rd(2)) }
	sub i32 { unpack('V', rd(4)) }
	my ($magic, $ver, $md) = (i32, u16, u16);
	$magic == 0x314E5552 or die "magic 不对（非检查点文件）";
	printf "version=%d mode=%d diff=%d\n", $ver, $md & 0xFF, ($md >> 8) & 0xFF;
	printf "seed=%d levelIndex=%d\n", i32, i32;
	printf "failCounts=%s\n", join(',', map { u16 } 1 .. 25);
	my $pn = u16; printf "pool(%d)=%s\n", $pn, join(',', map { u16 } 1 .. $pn);
	my $bn = u16;
	for my $i (1 .. $bn) { my ($id, $c) = (u16, u16); print "buff id=$id count=$c\n" }
	my $mower = u16; my $op = u16;
	printf "mowerUsedRows=0x%04X options=0x%04X (scale=%d tempo=%d zombotany=%d buffMult=%d ladder=%d zombieBuff=%d)\n",
		$mower, $op, $op & 3, ($op >> 2) & 3, ($op >> 4) & 1, ($op >> 5) & 3, ($op >> 7) & 7, ($op >> 10) & 3;
	printf "endlessScene=%d bossFlag=%d\n", u16, u16;
	if ($ver >= 14) {
		my @st = map { u16 } 1 .. 8;
		printf "zombieStacks=[%s]\n", join(',', @st);
		printf "zombieVotedLevel=%d (0xFFFF=没投过)\n", u16;
	} else {
		print "（v13 及更老档：到此为止，没有 v14 尾部字段 zombieStacks/zombieVotedLevel）\n";
	}
	if ($ver >= 15) { printf "vouchers=%d awardedLevel=%d\n", u16, unpack('l<', rd(4)) }
	printf "（ consumed=%d bytes / file=%d bytes ）\n", $o, length $d;
	exit 0;
}

# ── 造档：Save() 的 v14 字节布局 ─────────────────────────────────────────
defined $out or die "用法：--out <run%d.dat> [选项...] 或 --dump <run%d.dat>";
if (-e $out) {
	require File::Copy;
	File::Copy::copy($out, "$out.bak") or die "备份失败: $!";
	print "已备份原档 → $out.bak\n";
}

my $bits = ($OPT{scale} & 3) | (($OPT{tempo} & 3) << 2) | (($OPT{zombotany} & 1) << 4)
	| (($OPT{buffmult} & 3) << 5) | (($OPT{ladder} & 7) << 7) | (($OPT{zombiebuff} & 3) << 10);

my $data = '';
$data .= pack('V', 0x314E5552);                                  # magic 'RUN1'
$data .= pack('v', 15);                                          # version
$data .= pack('v', ($OPT{mode} & 0xFF) | (($OPT{diff} & 0xFF) << 8));
$data .= pack('V', $OPT{seed});
$data .= pack('V', $OPT{level});
$data .= pack('v', 0) x 25;                                      # failCounts 全零
$data .= pack('v', scalar @pool) . join('', map { pack('v', $_) } @pool);
$data .= pack('v', scalar @buffs) . join('', map { pack('vv', $_->[0], $_->[1]) } @buffs);
$data .= pack('v', 0);                                           # mowerUsedRows
$data .= pack('v', $bits);                                       # 高级选项位包
$data .= pack('v', $OPT{mode} == 3 ? $OPT{scene} : 0);           # 无尽场景（非无尽恒 0）
$data .= pack('v', $OPT{boss} ? 1 : 0);
my %stackmap = map { $_->[0] => $_->[1] } @stacks;
$data .= pack('v', $stackmap{$_} // 0) for 0 .. 7;               # 8 条层数
$data .= pack('v', $OPT{votedlevel});
$OPT{vouchers} >= 0 && $OPT{vouchers} <= 2 or die "vouchers 越界（0..2）";
$OPT{awardedlevel} >= -1 && $OPT{awardedlevel} <= $OPT{level} or die "awardedlevel 越界";
$data .= pack('vV', $OPT{vouchers}, $OPT{awardedlevel} & 0xFFFFFFFF);

open my $fh, '>:raw', $out or die "写不了 $out: $!";
print $fh $data; close $fh;
printf "已写入 %s（%d bytes）mode=%d diff=%d level=%d pool=[%s] buffs=[%s] ladder=%d zombieBuff=%d scene=%d\n",
	$out, length $data, $OPT{mode}, $OPT{diff}, $OPT{level}, join(',', @pool),
	join(',', map { "$_->[0]:$_->[1]" } @buffs), $OPT{ladder}, $OPT{zombiebuff},
	($OPT{mode} == 3 ? $OPT{scene} : 0);
print "下一步：游戏关着的状态替换 userdata 里的 run%d.dat → 开游戏 → 主菜单点「继续」。\n";
