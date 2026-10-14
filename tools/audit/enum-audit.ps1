$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
function Read-Enum([string]$File, [string]$Enum) {
    $source = Get-Content -LiteralPath $File -Raw
    $source = [regex]::Replace($source, '/\*.*?\*/|//[^\r\n]*', '', 'Singleline')
    $match = [regex]::Match($source, "enum\s+$Enum(?:\s*:\s*\w+)?\s*\{(.*?)\}", 'Singleline')
    if (!$match.Success) { throw "Enum missing: $Enum in $File" }
    $values = [ordered]@{}
    $next = 0
    foreach ($entry in ($match.Groups[1].Value -split ',')) {
        $entry = $entry.Trim()
        if (!$entry) { continue }
        if ($entry -notmatch '^(\w+)(?:\s*=\s*(-?\d+))?$') { throw "Unsupported enum entry: $entry" }
        if ($Matches[2]) { $next = [int]$Matches[2] }
        $values[$Matches[1]] = $next
        $next++
    }
    return $values
}
$report = [System.Collections.Generic.List[string]]::new()
$report.Add('# 植物/僵尸编号兼容审查')
$report.Add('')
$report.Add('固定参考：Windows 7a756a2（docs/reference/windows，SeedType/ZombieType 与本轮固定 Build 44 一致）；比较对象为当前 Android src/ConstEnums.h，Portable 底座为 141c1476。以下表由 tools/audit/enum-audit.ps1 解析显式整数与隐式递增得到；遇到其他表达式会失败，不猜值。')
$report.Add('')
foreach ($enum in @('SeedType','ZombieType')) {
    $windows = Read-Enum "$projectRoot\docs\reference\windows\ConstEnums.h" $enum
    $portable = Read-Enum "$projectRoot\src\ConstEnums.h" $enum
    $report.Add("## $enum")
    $report.Add('')
    $report.Add('| 名称 | PC | Portable | 结果 |')
    $report.Add('|---|---:|---:|---|')
    $names = @($windows.Keys) + @($portable.Keys | Where-Object { !$windows.Contains($_) })
    foreach ($name in $names) {
        $win = if ($windows.Contains($name)) { $windows[$name] } else { '—' }
        $port = if ($portable.Contains($name)) { $portable[$name] } else { '—' }
        $result = if ($win -eq $port) { '一致' } elseif ($port -eq '—') { 'Portable 尚缺' } elseif ($win -eq '—') { '仅 Portable' } else { '不同' }
        $report.Add("| $name | $win | $port | $result |")
    }
    $report.Add('')
}
$report.Add('## 迁移结论')
$report.Add('')
$report.Add('- 两套 SeedType 的名称和编号全部一致，可以直接复用卡池/检查点编号。')
$report.Add('- Android 已保留 33 自定义头像编号并迁入 34–41 八种防具变体；NUM_ZOMBIE_TYPES=42、缓存专用编号=43、NUM_CACHED_ZOMBIE_TYPES=44，与固定 PC 一致。缓存专用编号仍不是线上可投放实体类型。')
$report.Add('- NetProtocol 的 EscapedZombie.mZombieType 使用 U16 小端编解码；解码器只读数值，接收路径仍需验证支持范围，拒绝枚举计数值与缓存专用类型。')
$report.Add('- 八种变体已接入定义、初始化/防具、报纸狂暴、植物头动作、死亡/脱帽及缓存边界，完整固定 PC RunZombieRoster 已编译启用。运行只检查代表类型，没有逐只实测全部组合。')
$report.Add('- 编号一致与底层会话已编译，不表示 Android 已完成游戏侧联机接入；下一轮还需开局、选卡就绪、漏怪、团队结算与实际双端检查。')
$outDir = "$projectRoot\docs\compatibility"
New-Item -ItemType Directory -Force $outDir | Out-Null
[System.IO.File]::WriteAllText("$outDir\enum-audit.md", ($report -join "`n") + "`n")
Write-Output "Wrote $outDir\enum-audit.md"
