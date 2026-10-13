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
$report.Add('固定输入：Windows 7a756a2（docs/reference/windows），Portable 141c1476（src/ConstEnums.h）。以下表由 tools/audit/enum-audit.ps1 解析显式整数与隐式递增得到；遇到其他表达式会失败，不猜值。')
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
$report.Add('- 共同僵尸编号 0–33 一致；PC 新增 34–41 八种防具变体。Portable 的 NUM_ZOMBIE_TYPES 与缓存专用编号和 PC 不同，不能直接把这些计数/缓存值当线传实体类型。')
$report.Add('- NetProtocol 的 EscapedZombie.mZombieType 使用 U16 小端编解码（参考头 487/928/943 行）；解码器只读数值，类型支持必须由会话/战斗接收路径验证。')
$report.Add('- 新增僵尸需要同时迁入 Zombie 定义、初始化/防具行为、名称与缓存边界；只加枚举不能宣称类型已支持。在迁入之前不编译/启用包含这些类型的 RunZombieRoster。')
$report.Add('- 本轮先迁入 RunState 和 RunBuffs 数据。它们依赖共同植物编号，不需要为了编译先修改僵尸枚举。')
$outDir = "$projectRoot\docs\compatibility"
New-Item -ItemType Directory -Force $outDir | Out-Null
[System.IO.File]::WriteAllText("$outDir\enum-audit.md", ($report -join "`n") + "`n")
Write-Output "Wrote $outDir\enum-audit.md"
