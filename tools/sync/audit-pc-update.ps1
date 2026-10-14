param(
    [Parameter(Mandatory=$true)][string]$PcRepository,
    [string]$Candidate = 'HEAD',
    [string]$OutputDirectory
)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$baseline = Get-Content -LiteralPath "$projectRoot/docs/compatibility/pc-source-baseline.json" -Raw | ConvertFrom-Json
$pcPath = (Resolve-Path -LiteralPath $PcRepository).Path
function Read-Git([string[]]$Arguments) {
    $result = & git -c core.quotepath=false -C $pcPath @Arguments
    if ($LASTEXITCODE -ne 0) { throw "Cannot read committed PC source: git $($Arguments -join ' ')" }
    return ($result -join "`n")
}
$candidateCommit = (Read-Git @('rev-parse', '--verify', "$Candidate^{commit}")).Trim()
if ($candidateCommit -notmatch '^[0-9a-f]{40}$') { throw 'Cannot resolve candidate commit.' }
$null = Read-Git @('cat-file', '-e', "$($baseline.pcCommit)^{commit}")
$protocol = Read-Git @('show', "${candidateCommit}:Lawn/Online/NetProtocol.h")
$buildMatch = [regex]::Match($protocol, 'const\s+uint16_t\s+MOD_BUILD\s*=\s*(\d+)\s*;')
$versionMatch = [regex]::Match($protocol, 'const\s+uint16_t\s+PROTOCOL_VERSION\s*=\s*(\d+)\s*;')
if (!$buildMatch.Success -or !$versionMatch.Success) { throw 'Candidate protocol constants not found.' }
$diff = Read-Git @('diff', '--name-status', '--no-renames', $baseline.pcCommit, $candidateCommit, '--')
$changes = @(foreach ($line in ($diff -split "`n")) {
    if (!$line) { continue }
    $status, $path = $line -split "`t", 2
    $areas = @()
    if ($path -match '^Lawn/(Plant|Projectile|Zombie)\.(cpp|h)$') { $areas += '效果消费者、实体默认值、局内存档、漏怪/观战状态' }
    if ($path -match '^Lawn/Run/') { $areas += '闯关规则、确定性抽取、检查点兼容、迁移矩阵' }
    if ($path -match '^Lawn/Online/') { $areas += '线格式、会话语义、双端版本/能力核对' }
    if ($path -match '^LawnApp\.|^Lawn/Board\.') { $areas += '开局/就绪屏障、团队结算、难度、生命周期' }
    if ($path -match '^ConstEnums\.h$') { $areas += '枚举编号、定义/缓存边界、输入校验' }
    if ($path -match '^Lawn/Widget/') { $areas += 'Android 交互与中英文 UI 适配' }
    if (!$areas.Count) { $areas += '人工审查相关性，不自动复制无关 Windows 文件' }
    [pscustomobject]@{ status = $status; path = $path; review = ($areas -join '; ') }
})
if (!$OutputDirectory) { $OutputDirectory = "$projectRoot/docs/compatibility/updates" }
$outputPath = [IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Force -Path $outputPath | Out-Null
$report = [pscustomobject]@{
    schemaVersion = 1
    baselineCommit = $baseline.pcCommit
    baselineBuild = $baseline.pcModBuild
    candidateCommit = $candidateCommit
    candidateBuild = [int]$buildMatch.Groups[1].Value
    candidateProtocol = [int]$versionMatch.Groups[1].Value
    automaticallyApplied = $false
    changes = $changes
}
$short = $candidateCommit.Substring(0, 7)
$reportName = $baseline.pcCommit.Substring(0, 7) + '-to-' + $short
$encoding = [Text.UTF8Encoding]::new($false)
[IO.File]::WriteAllText("$outputPath/$reportName.json", ($report | ConvertTo-Json -Depth 6) + "`n", $encoding)
$lines = [Collections.Generic.List[string]]::new()
$lines.Add('# PC 更新候选审计')
$lines.Add('')
$lines.Add("当前固定来源：``$($baseline.pcCommit)`` / Build $($baseline.pcModBuild)。")
$lines.Add("候选已提交来源：``$candidateCommit`` / Build $($report.candidateBuild) / Protocol $($report.candidateProtocol)。")
$lines.Add('')
$lines.Add('本工具只读取提交，输出变更与审查范围；不修改 PC 工作区，不更新 Android 来源、不复制代码、不宣称新版本兼容。')
$lines.Add('')
$lines.Add('| 状态 | PC 文件 | Android 需核对 |')
$lines.Add('|---|---|---|')
foreach ($change in $changes) {
    $lines.Add("| $($change.status) | ``$($change.path)`` | $($change.review) |")
}
$lines.Add('')
$lines.Add('迁入消费者、实体默认值、存档/协议和 UI 依赖后，做受影响机制的代表检查，再推进 pc-source-baseline.json、生成 PortVersion.h 和双方交付记录。新关、新档与旧档恢复分开核对；不通过只改 Build 号来假装同步。')
[IO.File]::WriteAllText("$outputPath/$reportName.md", ($lines -join "`n") + "`n", $encoding)
Write-Output "Candidate Build $($report.candidateBuild): $($changes.Count) changed paths. Reports: $outputPath/$reportName.md and .json"
