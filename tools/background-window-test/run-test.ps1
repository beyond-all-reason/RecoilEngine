# Runs the BackgroundWindow test cases on Windows with window-focus-probe.ps1 and prints
# one result per case. The cases and what they check are described in
# doc/site/content/docs/guides/getting-started/background-window.md.
#
#   powershell -NoProfile -ExecutionPolicy Bypass -File run-test.ps1 -Engine C:\path\to\spring.exe [-Seconds 25] [-OutDir dir] [-Cases off,switch,fullscreen,config]
#
# Start it from the window that has the focus and keep that window focused. Exit code 0
# when every case with the feature passes.
param(
	[Parameter(Mandatory = $true)][string]$Engine,
	[int]$Seconds = 25,
	[string]$OutDir = (Join-Path $env:TEMP "recoil-background-window-test"),
	[string]$Cases = "off,switch,fullscreen,config"
)

if (-not (Test-Path -PathType Leaf $Engine)) { Write-Output "engine not found: $Engine"; exit 2 }
$Engine = (Resolve-Path $Engine).Path
$engineDir = Split-Path -Parent $Engine
$probe = Join-Path $PSScriptRoot "window-focus-probe.ps1"
$logLine = "background window: shown without activation, lowered, never minimized"
$baseConfig = @("XResolutionWindowed = 1280", "YResolutionWindowed = 720", "WindowPosX = 40", "WindowPosY = 40", "WindowBorderless = 0", "LogFlush = 1")

$results = @()
foreach ($case in ($Cases -split ",")) {
	$config = $baseConfig
	$extraArgs = @()
	switch ($case) {
		"off"        { $config += "Fullscreen = 0" }
		"switch"     { $config += "Fullscreen = 0"; $extraArgs = @("--background-window") }
		"fullscreen" { $config += "Fullscreen = 1"; $extraArgs = @("--background-window") }
		"config"     { $config += @("Fullscreen = 0", "BackgroundWindow = 1") }
		default      { Write-Output "unknown case $case"; exit 2 }
	}
	$writeDir = Join-Path $OutDir $case
	if (Test-Path $writeDir) { Remove-Item -Recurse -Force $writeDir }
	New-Item -ItemType Directory -Force $writeDir | Out-Null
	$cfgFile = Join-Path $writeDir "springsettings.cfg"
	$config | Out-File -Encoding ascii $cfgFile

	Write-Output "== $case : spring.exe $($extraArgs -join ' ')"
	$engineArgs = @("--isolation", "--isolation-dir", "`"$engineDir`"", "--write-dir", "`"$writeDir`"") + $extraArgs
	$p = Start-Process -FilePath $Engine -ArgumentList $engineArgs -PassThru
	try {
		$probeOut = & powershell -NoProfile -ExecutionPolicy Bypass -File $probe -ProcessId $p.Id -Seconds $Seconds -Csv (Join-Path $writeDir "probe.csv")
	} finally {
		Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue
	}
	$probeOut | ForEach-Object { Write-Output "   $_" }

	$verdict = ($probeOut | Select-String "VERDICT background: (\S+)").Matches.Groups[1].Value
	$infolog = Join-Path $writeDir "infolog.txt"
	$hasLine = (Test-Path $infolog) -and (Select-String -Quiet -SimpleMatch $logLine $infolog)
	$windowed = "-"
	if ($case -eq "fullscreen") {
		# the engine logs the mode it set; it must not have saved Fullscreen = 0 either
		$modes = @(Select-String -Path $infolog -Pattern "display-mode set to .*\((\w+)::" | ForEach-Object { $_.Matches.Groups[1].Value })
		$windowed = ($modes.Count -gt 0) -and -not ($modes -contains "fullscreen") -and -not (Select-String -Quiet "^\s*Fullscreen\s*=\s*0" $cfgFile)
	}
	$result = if ($case -eq "off") { "-" } elseif ($verdict -eq "PASS" -and $hasLine -and $windowed -ne $false) { "PASS" } else { "FAIL" }
	$results += [pscustomobject]@{ Case = $case; Probe = $verdict; InfologLine = $hasLine; Windowed = $windowed; Result = $result }
}

$results | Format-Table -AutoSize | Out-String | Write-Output
Write-Output "write directories, infologs and probe CSVs: $OutDir"
if ($results | Where-Object { $_.Result -eq "FAIL" }) { exit 1 }
exit 0
