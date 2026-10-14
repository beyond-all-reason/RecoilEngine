[CmdletBinding()]
param(
    [string] $BuildDirectory,
    [string] $GameDirectory,
    [string] $BARDataDirectory,
    [string] $StartScript,
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

New-Item -ItemType Directory -Force -Path $gamesDirectory | Out-Null
New-Item -ItemType Directory -Force -Path $writeDirectory | Out-Null
Ensure-Junction -Path $gameMount -Target $GameDirectory
Ensure-Junction -Path $mapMount -Target $installedMaps

Write-Host "Launching isolated PBR IBL smoke test"
Write-Host "  Engine:      $engine"
Write-Host "  Game:        $GameDirectory"
Write-Host "  Maps:        $installedMaps"
Write-Host "  Writable:    $writeDirectory"
Write-Host "  Startscript: $StartScript"

if ($PrepareOnly) {
    Write-Host 'Runtime prepared; launch skipped because -PrepareOnly was supplied.'
    return
}

$engineArguments = @(
    '--isolation',
    '--isolation-dir', $BuildDirectory,
    '--write-dir', $writeDirectory,
    '-window',
    $StartScript
)

& $engine @engineArguments
exit $LASTEXITCODE
