[CmdletBinding()]
param(
    [string] $BuildDirectory,
    [string] $MingwLibsDirectory
)

$ErrorActionPreference = 'Stop'

$recoilRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..\..')).Path
if ([string]::IsNullOrWhiteSpace($BuildDirectory)) {
    $BuildDirectory = Join-Path $recoilRoot 'build\pbr-window'
}
if ([string]::IsNullOrWhiteSpace($MingwLibsDirectory)) {
    $MingwLibsDirectory = Join-Path $recoilRoot 'mingwlibs64'
}

$BuildDirectory = (Resolve-Path -LiteralPath $BuildDirectory).Path
$MingwLibsDirectory = (Resolve-Path -LiteralPath $MingwLibsDirectory).Path
$sourceDirectory = Join-Path $MingwLibsDirectory 'dll'
$engine = Join-Path $BuildDirectory 'spring.exe'

if (-not (Test-Path -LiteralPath $engine -PathType Leaf)) {
    throw "The engine has not been built: $engine"
}
if (-not (Test-Path -LiteralPath $sourceDirectory -PathType Container)) {
    throw "The MinGW runtime DLL directory does not exist: $sourceDirectory"
}

$runtimeLibraries = @(Get-ChildItem -LiteralPath $sourceDirectory -Filter '*.dll' -File)
if ($runtimeLibraries.Count -eq 0) {
    throw "No runtime DLLs were found in $sourceDirectory"
}

foreach ($library in $runtimeLibraries) {
    Copy-Item -LiteralPath $library.FullName -Destination (Join-Path $BuildDirectory $library.Name) -Force
}

$requiredLibraries = @(
    'libcurl.dll',
    'OpenAL32.dll',
    'zlib1.dll',
    'SDL2.dll',
    'libfontconfig-1.dll',
    'libfreetype-6.dll',
    'libIL.dll',
    'libvorbisfile-3.dll'
)
foreach ($library in $requiredLibraries) {
    $installedPath = Join-Path $BuildDirectory $library
    if (-not (Test-Path -LiteralPath $installedPath -PathType Leaf)) {
        throw "Required runtime DLL was not installed: $installedPath"
    }
}

Write-Host "Installed and verified $($runtimeLibraries.Count) runtime DLLs in $BuildDirectory"
