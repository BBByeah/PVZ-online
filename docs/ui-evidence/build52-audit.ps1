param([string]$OutputPath = (Join-Path $PSScriptRoot 'build52-audit.json'))
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
$repoRoot = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$artifactRoot = Join-Path (Split-Path $repoRoot -Parent) 'android-artifacts'

function Get-UiLuminance([int[]]$Rgb) {
    $channels = @($Rgb | ForEach-Object {
        $channel = $_ / 255.0
        if ($channel -le 0.04045) { $channel / 12.92 }
        else { [Math]::Pow(($channel + 0.055) / 1.055, 2.4) }
    })
    return .2126 * $channels[0] + .7152 * $channels[1] + .0722 * $channels[2]
}
function Get-UiContrast([double]$First, [double]$Second) {
    return [Math]::Round(([Math]::Max($First, $Second) + .05) /
        ([Math]::Min($First, $Second) + .05), 3)
}
$currentInk = @(30, 75, 15)
$proposedInk = @(245, 230, 165)
$bitmap = [Drawing.Bitmap]::new((Join-Path $artifactRoot 'coop52/loan-ui.png'))
try {
    $samples = @(foreach ($point in @(@(366,351), @(400,375), @(850,350), @(850,380))) {
        $pixel = $bitmap.GetPixel($point[0], $point[1])
        $background = @([int]$pixel.R, [int]$pixel.G, [int]$pixel.B)
        $luminance = Get-UiLuminance $background
        [ordered]@{
            screenshotPixel = $point
            backgroundRgb = $background
            currentContrast = Get-UiContrast $luminance (Get-UiLuminance $currentInk)
            proposedContrast = Get-UiContrast $luminance (Get-UiLuminance $proposedInk)
        }
    })
} finally { $bitmap.Dispose() }
$evidence = @(foreach ($relative in @('coop52/loan-ui.png', 'coop52/loan-result.png',
    'coop52/battle.png', 'build51-sync/room-final.png', 'build51-sync/menu.png')) {
    $path = Join-Path $artifactRoot $relative
    $item = Get-Item -LiteralPath $path
    $bitmap = [Drawing.Bitmap]::new($path)
    try {
        [ordered]@{ pathRelativeToArtifacts = $relative; width = $bitmap.Width;
            height = $bitmap.Height; bytes = $item.Length;
            sha256 = (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash }
    } finally { $bitmap.Dispose() }
})
$report = [ordered]@{
    date = '2026-10-15'
    androidBaseline = '1ea2a20'
    evidenceType = 'Existing screenshots reviewed; no game launched or modified UI rendered.'
    coordinateMapping = @{ screenshotWidth = 1920; screenshotHeight = 1080;
        gameRect = @(240, 0, 1440, 1080); logicalSize = @(800, 600); scale = 1.8 }
    colorMethod = 'sRGB relative luminance: sampled button background vs source ink, no glyph-edge sampling.'
    currentInkRgb = $currentInk
    proposedInkRgb = $proposedInk
    samples = $samples
    evidence = $evidence
    limits = @('No complete texture contrast audit.', 'No post-change screenshot.',
        'Build52 rewards/mode/resource/watch pages are source-only inspection.')
}
[IO.File]::WriteAllText([IO.Path]::GetFullPath($OutputPath),
    ($report | ConvertTo-Json -Depth 8) + "`n", [Text.UTF8Encoding]::new($false))
Write-Output "Recorded $($samples.Count) background samples and $($evidence.Count) screenshot hashes: $OutputPath"
