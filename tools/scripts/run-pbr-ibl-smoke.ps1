[CmdletBinding()]
param(
    [string] $BuildDirectory,
    [string] $GameDirectory,
    [string] $BARDataDirectory,
    [string] $StartScript,
    [string] $GameArchiveName = 'Beyond All Reason $VERSION',
    [ValidateSet('legacy-clone', 'hdr-modern', 'compare')]
    [string] $EnvironmentCapture = 'hdr-modern',
    [switch] $PrepareOnly
)

$ErrorActionPreference = 'Stop'

$recoilRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..\..')).Path
$workspaceRoot = Split-Path -Parent $recoilRoot

if ([string]::IsNullOrWhiteSpace($BuildDirectory)) {
    $BuildDirectory = Join-Path $recoilRoot 'build\pbr-window'
}
if ([string]::IsNullOrWhiteSpace($GameDirectory)) {
    $GameDirectory = Join-Path $workspaceRoot 'Beyond-All-Reason-Private-pbr-rendering-V3.sdd'
}
if ([string]::IsNullOrWhiteSpace($BARDataDirectory)) {
    $BARDataDirectory = Join-Path $workspaceRoot 'BARS-Install\data'
}
if ([string]::IsNullOrWhiteSpace($StartScript)) {
    $StartScript = Join-Path $BARDataDirectory '_script.txt'
}

$BuildDirectory = (Resolve-Path -LiteralPath $BuildDirectory).Path
$GameDirectory = (Resolve-Path -LiteralPath $GameDirectory).Path
$BARDataDirectory = (Resolve-Path -LiteralPath $BARDataDirectory).Path
$StartScript = (Resolve-Path -LiteralPath $StartScript).Path

$engine = Join-Path $BuildDirectory 'spring.exe'
$baseDirectory = Join-Path $BuildDirectory 'base'
$installedMaps = Join-Path $BARDataDirectory 'maps'
$gamesDirectory = Join-Path $BuildDirectory 'games'
$gameMount = Join-Path $gamesDirectory 'Beyond-All-Reason-Private-pbr-rendering-V3.sdd'
$mapMount = Join-Path $BuildDirectory 'maps'
$writeDirectory = Join-Path $BuildDirectory 'pbr-user'
$runtimeStartScript = Join-Path $writeDirectory '_script-pbr-v3.txt'
$runtimeConfig = Join-Path $writeDirectory 'springsettings.cfg'
$runtimeLibraries = @(
    'libcurl.dll',
    'OpenAL32.dll',
    'zlib1.dll',
    'SDL2.dll',
    'libfontconfig-1.dll',
    'libfreetype-6.dll',
    'libIL.dll',
    'libvorbisfile-3.dll'
) | ForEach-Object { Join-Path $BuildDirectory $_ }

foreach ($requiredPath in @($engine, $baseDirectory, $installedMaps) + $runtimeLibraries) {
    if (-not (Test-Path -LiteralPath $requiredPath)) {
        throw "Required PBR smoke-test path does not exist: $requiredPath. Run the VS Code task 'PBR: debug-ready' or tools/scripts/install-pbr-runtime.ps1 first."
    }
}

function Ensure-Junction {
    param(
        [Parameter(Mandatory)] [string] $Path,
        [Parameter(Mandatory)] [string] $Target
    )

    if (Test-Path -LiteralPath $Path) {
        $item = Get-Item -LiteralPath $Path -Force
        $actualTargets = @($item.Target | ForEach-Object { [System.IO.Path]::GetFullPath($_) })
        $expectedTarget = [System.IO.Path]::GetFullPath($Target)
        if ($item.LinkType -ne 'Junction' -or $actualTargets -notcontains $expectedTarget) {
            throw "Refusing to replace existing non-matching path: $Path"
        }
        return
    }

    New-Item -ItemType Junction -Path $Path -Target $Target | Out-Null
}

function Set-SpringSetting {
    param(
        [Parameter(Mandatory)] [string] $ConfigText,
        [Parameter(Mandatory)] [string] $Name,
        [Parameter(Mandatory)] [string] $Value
    )

    $settingRegex = [regex]::new('(?im)^\s*' + [regex]::Escape($Name) + '\s*=.*$')
    $matches = $settingRegex.Matches($ConfigText)
    if ($matches.Count -gt 1) {
        throw "Expected at most one $Name entry in the isolated runtime configuration: $runtimeConfig"
    }

    $setting = "$Name = $Value"
    if ($matches.Count -eq 1) {
        return $settingRegex.Replace($ConfigText, $setting, 1)
    }

    if ($ConfigText.Length -gt 0 -and -not $ConfigText.EndsWith("`n")) {
        $ConfigText += [Environment]::NewLine
    }
    return $ConfigText + $setting + [Environment]::NewLine
}

New-Item -ItemType Directory -Force -Path $gamesDirectory | Out-Null
New-Item -ItemType Directory -Force -Path $writeDirectory | Out-Null
Ensure-Junction -Path $gameMount -Target $GameDirectory
Ensure-Junction -Path $mapMount -Target $installedMaps

$sourceScriptText = [System.IO.File]::ReadAllText($StartScript)
$gameTypeRegex = [regex]::new('(?im)^(\s*gametype\s*=\s*)[^;]+;')
if ($gameTypeRegex.Matches($sourceScriptText).Count -ne 1) {
    throw "Expected exactly one gametype entry in the startscript: $StartScript"
}
$runtimeScriptText = $gameTypeRegex.Replace(
    $sourceScriptText,
    [System.Text.RegularExpressions.MatchEvaluator] {
        param($match)
        return $match.Groups[1].Value + $GameArchiveName + ';'
    },
    1
)
[System.IO.File]::WriteAllText($runtimeStartScript, $runtimeScriptText, [System.Text.UTF8Encoding]::new($false))

$runtimeConfigText = if (Test-Path -LiteralPath $runtimeConfig) {
    [System.IO.File]::ReadAllText($runtimeConfig)
} else {
    ''
}
$runtimeConfigText = Set-SpringSetting -ConfigText $runtimeConfigText -Name 'Fullscreen' -Value '0'
$runtimeConfigText = Set-SpringSetting -ConfigText $runtimeConfigText -Name 'PBREnvironmentCapture' -Value $EnvironmentCapture
[System.IO.File]::WriteAllText($runtimeConfig, $runtimeConfigText, [System.Text.UTF8Encoding]::new($false))

Write-Host "Launching isolated PBR IBL smoke test"
Write-Host "  Engine:      $engine"
Write-Host "  Game:        $GameDirectory"
Write-Host "  Maps:        $installedMaps"
Write-Host "  Writable:    $writeDirectory"
Write-Host "  Source script:  $StartScript"
Write-Host "  Runtime script: $runtimeStartScript"
Write-Host "  Game archive:   $GameArchiveName"
Write-Host "  PBR capture:    $EnvironmentCapture"

if ($PrepareOnly) {
    Write-Host 'Runtime prepared; launch skipped because -PrepareOnly was supplied.'
    return
}

$engineArguments = @(
    '--isolation',
    '--isolation-dir', $BuildDirectory,
    '--write-dir', $writeDirectory,
    '-window',
    $runtimeStartScript
)

& $engine @engineArguments
exit $LASTEXITCODE
