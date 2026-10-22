param([string]$ResourceDirectory, [string]$OutputDirectory)
$ErrorActionPreference = 'Stop'
$projectDir = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$taskRoot = Split-Path $projectDir -Parent
if (!$ResourceDirectory) { $ResourceDirectory = Join-Path $taskRoot 'runtime' }
if (!$OutputDirectory) { $OutputDirectory = Join-Path $taskRoot 'android-artifacts\bundled-game-build57\assets' }
foreach ($required in @('main.pak', 'pvz_zh.pak', 'properties')) {
    if (!(Test-Path -LiteralPath (Join-Path $ResourceDirectory $required))) {
        throw "Full player APK requires $required in $ResourceDirectory. Supply local game resources before packaging."
    }
}
# @pvz-online: only game resources enter the APK, never userdata or diagnostic files.
$bundle = Join-Path $OutputDirectory 'bundled-game'
New-Item -ItemType Directory -Force $bundle | Out-Null
$resourceRoot = (Resolve-Path -LiteralPath $ResourceDirectory).Path
$files = @((Get-Item -LiteralPath (Join-Path $resourceRoot 'main.pak')), (Get-Item -LiteralPath (Join-Path $resourceRoot 'pvz_zh.pak')))
$files += @(Get-ChildItem -LiteralPath (Join-Path $resourceRoot 'properties') -Recurse -File)
$manifest = foreach ($file in ($files | Sort-Object FullName)) {
    $relative = $file.FullName.Substring($resourceRoot.Length + 1).Replace('\', '/')
    $target = Join-Path $bundle $relative
    New-Item -ItemType Directory -Force (Split-Path $target -Parent) | Out-Null
    Copy-Item -LiteralPath $file.FullName -Destination $target -Force
    $sha = (Get-FileHash -LiteralPath $target -Algorithm SHA256).Hash.ToLowerInvariant()
    "$sha`t$relative`t$($file.Length)"
}
[IO.File]::WriteAllLines((Join-Path $bundle 'manifest.tsv'), [string[]]$manifest, [Text.UTF8Encoding]::new($false))
Write-Output "Full APK resources: $($files.Count) files; assets=$OutputDirectory"
