<#
.SYNOPSIS
	Samples whether the engine window disturbs the user, without sending any input.

.DESCRIPTION
	Every -IntervalMs it records whether the engine window has the focus, is minimized, is
	above the focused window, or has been replaced by a "Not Responding" ghost window.
	The test cases and how to run them are in
	doc/site/content/docs/guides/getting-started/background-window.md.

	The last line is one of:
	  VERDICT background: PASS        window seen; engine_foreground, ghost_foreground,
	                                  minimized and above_foreground all 0
	  VERDICT background: FAIL
	  VERDICT background: EXITED      the engine ended before -Seconds
	  VERDICT background: NO-PROCESS  no process with -ProcessId

	"above" is not measured while the shell (taskbar, Start, search) or nothing has the focus;
	those samples are counted as shell_foreground and no_foreground.

.EXAMPLE
	powershell -NoProfile -ExecutionPolicy Bypass -File window-focus-probe.ps1 -ProcessId 1234 -Seconds 30 -Csv probe.csv
#>
param(
	[Parameter(Mandatory = $true)]
	[int]$ProcessId,

	[int]$Seconds = 30,

	[int]$IntervalMs = 200,

	# one row per sample, written at the end; empty for none
	[string]$Csv = ""
)

Add-Type @"
using System;
using System.Text;
using System.Runtime.InteropServices;

public static class WindowFocusProbe
{
	[DllImport("user32.dll")]
	public static extern IntPtr GetForegroundWindow();

	[DllImport("user32.dll")]
	public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint pid);

	[DllImport("user32.dll", CharSet = CharSet.Unicode)]
	public static extern int GetClassName(IntPtr hWnd, StringBuilder name, int max);

	[DllImport("user32.dll")]
	public static extern bool IsHungAppWindow(IntPtr hWnd);

	[DllImport("user32.dll")]
	public static extern bool IsIconic(IntPtr hWnd);

	[DllImport("user32.dll")]
	public static extern IntPtr GetTopWindow(IntPtr hWnd);

	[DllImport("user32.dll")]
	public static extern IntPtr GetWindow(IntPtr hWnd, uint cmd);

	// WinUser.h: walks the z-order downwards
	public const uint GW_HWNDNEXT = 2;

	// the maximum window class name length
	public const int CLASS_NAME_MAX = 256;

	public static string ClassOf(IntPtr h)
	{
		var name = new StringBuilder(CLASS_NAME_MAX);
		GetClassName(h, name, CLASS_NAME_MAX);
		return name.ToString();
	}

	// true when window a comes before window b walking the z-order from the top
	public static bool IsAbove(IntPtr a, IntPtr b)
	{
		for (IntPtr h = GetTopWindow(IntPtr.Zero); h != IntPtr.Zero; h = GetWindow(h, GW_HWNDNEXT))
		{
			if (h == a)
				return true;
			if (h == b)
				return false;
		}

		return false;
	}
}
"@

# Owners of the taskbar, Start, search, quick settings and notification surfaces. Other apps
# that use the same window classes (UWP windows are also Windows.UI.Core.CoreWindow) are
# measured normally.
$ShellOwners = @(
	"explorer",
	"StartMenuExperienceHost",
	"ShellExperienceHost",
	"SearchHost",
	"SearchApp",
	"ShellHost"
)

$process = Get-Process -Id $ProcessId -ErrorAction SilentlyContinue

if (-not $process)
{
	Write-Output "VERDICT background: NO-PROCESS"
	exit 2
}

$counts = [ordered]@{
	samples           = 0
	window_seen       = 0
	engine_foreground = 0
	ghost_foreground  = 0
	hung              = 0
	minimized         = 0
	above_foreground  = 0
	shell_foreground  = 0
	no_foreground     = 0
}

$rows = New-Object System.Collections.Generic.List[string]
$rows.Add("time,fg_hwnd,fg_owner,fg_class,engine_hwnd,engine_fg,hung,minimized,above_fg")
$exited = $false
$end = (Get-Date).AddSeconds($Seconds)

while ((Get-Date) -lt $end)
{
	$engine = Get-Process -Id $ProcessId -ErrorAction SilentlyContinue

	if (-not $engine)
	{
		$exited = $true
		break
	}

	$fg = [WindowFocusProbe]::GetForegroundWindow()
	[uint32]$fgPid = 0
	[void][WindowFocusProbe]::GetWindowThreadProcessId($fg, [ref]$fgPid)
	$fgClass = [WindowFocusProbe]::ClassOf($fg)
	$fgOwner = (Get-Process -Id $fgPid -ErrorAction SilentlyContinue).ProcessName
	$isShell = $ShellOwners -contains $fgOwner

	if ($fg -eq [IntPtr]::Zero)
	{
		$counts.no_foreground++
	}

	if ($isShell)
	{
		$counts.shell_foreground++
	}

	if ($fgClass -eq "Ghost")
	{
		$counts.ghost_foreground++
	}

	$window = $engine.MainWindowHandle
	$isFg = 0
	$isHung = 0
	$isMin = 0
	$isAbove = 0

	if ($window -ne [IntPtr]::Zero)
	{
		$counts.window_seen++

		if ($fg -eq $window)
		{
			$isFg = 1
			$counts.engine_foreground++
		}

		if ([WindowFocusProbe]::IsHungAppWindow($window))
		{
			$isHung = 1
			$counts.hung++
		}

		if ([WindowFocusProbe]::IsIconic($window))
		{
			$isMin = 1
			$counts.minimized++
		}

		$canCompare = ($isFg -eq 0) -and (-not $isShell) -and ($fg -ne [IntPtr]::Zero)

		if ($canCompare -and [WindowFocusProbe]::IsAbove($window, $fg))
		{
			$isAbove = 1
			$counts.above_foreground++
		}
	}

	$counts.samples++
	$time = Get-Date -Format HH:mm:ss.fff
	$rows.Add("$time,$fg,$fgOwner,$fgClass,$window,$isFg,$isHung,$isMin,$isAbove")
	Start-Sleep -Milliseconds $IntervalMs
}

if ($Csv)
{
	$rows | Out-File -Encoding utf8 $Csv
}

Write-Output (($counts.GetEnumerator() | ForEach-Object { "$($_.Key)=$($_.Value)" }) -join " ")

if ($exited)
{
	Write-Output "VERDICT background: EXITED"
	exit 1
}

$pass = ($counts.window_seen -gt 0) -and
	($counts.engine_foreground -eq 0) -and
	($counts.ghost_foreground -eq 0) -and
	($counts.minimized -eq 0) -and
	($counts.above_foreground -eq 0)

if ($pass)
{
	Write-Output "VERDICT background: PASS"
}
else
{
	Write-Output "VERDICT background: FAIL"
}
