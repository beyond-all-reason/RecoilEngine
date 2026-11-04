<#
.SYNOPSIS
	Runs the BackgroundWindow test cases on Windows and prints one result per case.

.DESCRIPTION
	Starts the engine once per case, without a game or map (it opens its built-in menu), and
	samples its window with window-focus-probe.ps1. The cases and what they check are described
	in doc/site/content/docs/guides/getting-started/background-window.md.

	Start it from the window that has the focus and keep that window focused.
	Exit code 0 when every case with the feature passes, 1 when one fails, 2 on a usage error.

.EXAMPLE
	powershell -NoProfile -ExecutionPolicy Bypass -File run-test.ps1 -Engine C:\path\to\spring.exe

.EXAMPLE
	powershell -NoProfile -ExecutionPolicy Bypass -File run-test.ps1 -Engine C:\path\to\spring.exe -Seconds 40 -Cases switch,config
#>
param(
	# the spring.exe to test
	[Parameter(Mandatory = $true)]
	[string]$Engine,

	# how long each case is sampled
	[int]$Seconds = 25,

	# one write directory per case is created below this folder
	[string]$OutDir = (Join-Path $env:TEMP "recoil-background-window-test"),

	# comma-separated subset of: off, switch, fullscreen, config
	[string]$Cases = "off,switch,fullscreen,config"
)

$LogLine = "background window: shown without activation, lowered, never minimized"

$BaseConfig = @(
	"XResolutionWindowed = 1280",
	"YResolutionWindowed = 720",
	"WindowPosX = 40",
	"WindowPosY = 40",
	"WindowBorderless = 0",
	"LogFlush = 1"
)

# Returns the extra springsettings.cfg lines and command-line arguments of one case.
function Get-CaseSetup([string]$Case)
{
	switch ($Case)
	{
		"off"
		{
			return @{ Config = @("Fullscreen = 0"); Arguments = @() }
		}

		"switch"
		{
			return @{ Config = @("Fullscreen = 0"); Arguments = @("--background-window") }
		}

		"fullscreen"
		{
			return @{ Config = @("Fullscreen = 1"); Arguments = @("--background-window") }
		}

		"config"
		{
			return @{ Config = @("Fullscreen = 0", "BackgroundWindow = 1"); Arguments = @() }
		}
	}

	return $null
}

# The fullscreen case also needs only windowed display modes in the infolog, and the engine
# must not have saved Fullscreen = 0 (the switch must not change the user's setting).
function Test-StayedWindowed([string]$Infolog, [string]$ConfigFile)
{
	$modes = @(
		Select-String -Path $Infolog -Pattern "display-mode set to .*\((\w+)::" |
			ForEach-Object { $_.Matches.Groups[1].Value }
	)

	if ($modes.Count -eq 0)
	{
		return $false
	}

	if ($modes -contains "fullscreen")
	{
		return $false
	}

	$savedOff = Select-String -Quiet -Path $ConfigFile -Pattern "^\s*Fullscreen\s*=\s*0"
	return -not $savedOff
}

if (-not (Test-Path -PathType Leaf $Engine))
{
	Write-Output "engine not found: $Engine"
	exit 2
}

$Engine = (Resolve-Path $Engine).Path
$engineDir = Split-Path -Parent $Engine
$probe = Join-Path $PSScriptRoot "window-focus-probe.ps1"

$results = @()

foreach ($case in ($Cases -split ","))
{
	$setup = Get-CaseSetup $case

	if (-not $setup)
	{
		Write-Output "unknown case $case"
		exit 2
	}

	$writeDir = Join-Path $OutDir $case

	if (Test-Path $writeDir)
	{
		Remove-Item -Recurse -Force $writeDir
	}

	New-Item -ItemType Directory -Force $writeDir | Out-Null
	$configFile = Join-Path $writeDir "springsettings.cfg"
	($BaseConfig + $setup.Config) | Out-File -Encoding ascii $configFile

	Write-Output "== $case : spring.exe $($setup.Arguments -join ' ')"
	$engineArgs = @(
		"--isolation",
		"--isolation-dir", "`"$engineDir`"",
		"--write-dir", "`"$writeDir`""
	) + $setup.Arguments
	$process = Start-Process -FilePath $Engine -ArgumentList $engineArgs -PassThru

	try
	{
		$probeOut = & powershell -NoProfile -ExecutionPolicy Bypass -File $probe `
			-ProcessId $process.Id `
			-Seconds $Seconds `
			-Csv (Join-Path $writeDir "probe.csv")
	}
	finally
	{
		Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue
	}

	$probeOut | ForEach-Object { Write-Output "   $_" }

	$verdict = ($probeOut | Select-String "VERDICT background: (\S+)").Matches.Groups[1].Value
	$infolog = Join-Path $writeDir "infolog.txt"
	$hasLine = (Test-Path $infolog) -and (Select-String -Quiet -SimpleMatch $LogLine $infolog)

	$windowed = "-"

	if ($case -eq "fullscreen")
	{
		$windowed = Test-StayedWindowed $infolog $configFile
	}

	if ($case -eq "off")
	{
		# the control: shows what the probe sees without the feature, never fails
		$result = "-"
	}
	elseif ($verdict -eq "PASS" -and $hasLine -and $windowed -ne $false)
	{
		$result = "PASS"
	}
	else
	{
		$result = "FAIL"
	}

	$results += [pscustomobject]@{
		Case        = $case
		Probe       = $verdict
		InfologLine = $hasLine
		Windowed    = $windowed
		Result      = $result
	}
}

$results | Format-Table -AutoSize | Out-String | Write-Output
Write-Output "write directories, infologs and probe CSVs: $OutDir"

if ($results | Where-Object { $_.Result -eq "FAIL" })
{
	exit 1
}

exit 0
