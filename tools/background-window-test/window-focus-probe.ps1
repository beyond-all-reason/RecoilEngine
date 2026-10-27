# Samples, without sending any input, whether the engine window takes the focus, is
# minimized, covers the focused window or is replaced by a "Not Responding" ghost window.
# The test cases and how to run them are in
# doc/site/content/docs/guides/getting-started/background-window.md.
#
#   powershell -NoProfile -ExecutionPolicy Bypass -File window-focus-probe.ps1 -ProcessId <pid> [-Seconds 30] [-IntervalMs 200] [-Csv out.csv]
#
# Last line: VERDICT background: PASS | FAIL | EXITED (the engine ended before -Seconds) | NO-PROCESS.
# PASS needs the window seen and engine_foreground, ghost_foreground, minimized and
# above_foreground all 0. "above" is not measured while the shell (taskbar, Start, search)
# or nothing has the focus; those samples are counted as shell_foreground and no_foreground.
param(
	[Parameter(Mandatory = $true)][int]$ProcessId,
	[int]$Seconds = 30,
	[int]$IntervalMs = 200,
	[string]$Csv = ""
)

# GW_HWNDNEXT (WinUser.h) walks the z-order downwards; 256 is the maximum class-name length
Add-Type @"
using System;
using System.Text;
using System.Runtime.InteropServices;
public static class WindowFocusProbe {
	[DllImport("user32.dll")] public static extern IntPtr GetForegroundWindow();
	[DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint pid);
	[DllImport("user32.dll", CharSet = CharSet.Unicode)] public static extern int GetClassName(IntPtr hWnd, StringBuilder name, int max);
	[DllImport("user32.dll")] public static extern bool IsHungAppWindow(IntPtr hWnd);
	[DllImport("user32.dll")] public static extern bool IsIconic(IntPtr hWnd);
	[DllImport("user32.dll")] public static extern IntPtr GetTopWindow(IntPtr hWnd);
	[DllImport("user32.dll")] public static extern IntPtr GetWindow(IntPtr hWnd, uint cmd);
	public const uint GW_HWNDNEXT = 2;
	public const int CLASS_NAME_MAX = 256;
	public static string ClassOf(IntPtr h) { var b = new StringBuilder(CLASS_NAME_MAX); GetClassName(h, b, CLASS_NAME_MAX); return b.ToString(); }
	public static bool IsAbove(IntPtr a, IntPtr b) {
		for (IntPtr h = GetTopWindow(IntPtr.Zero); h != IntPtr.Zero; h = GetWindow(h, GW_HWNDNEXT)) {
			if (h == a) return true;
			if (h == b) return false;
		}
		return false;
	}
}
"@

# owners of taskbar, Start, search, quick settings and notification surfaces; other apps that use the same
# window classes (UWP windows are also Windows.UI.Core.CoreWindow) are measured normally
$shellOwners = @("explorer", "StartMenuExperienceHost", "ShellExperienceHost", "SearchHost", "SearchApp", "ShellHost")

$process = Get-Process -Id $ProcessId -ErrorAction SilentlyContinue
if (-not $process) { Write-Output "VERDICT background: NO-PROCESS"; exit 2 }

$end = (Get-Date).AddSeconds($Seconds)
$rows = New-Object System.Collections.Generic.List[string]
$rows.Add("time,fg_hwnd,fg_owner,fg_class,engine_hwnd,engine_fg,hung,minimized,above_fg")
$n = 0; $seen = 0; $engineFg = 0; $ghostFg = 0; $hung = 0; $minimized = 0; $above = 0; $shellFg = 0; $noFg = 0
$exited = $false
while ((Get-Date) -lt $end) {
	$p = Get-Process -Id $ProcessId -ErrorAction SilentlyContinue
	if (-not $p) { $exited = $true; break }
	$fg = [WindowFocusProbe]::GetForegroundWindow()
	[uint32]$fgPid = 0
	[void][WindowFocusProbe]::GetWindowThreadProcessId($fg, [ref]$fgPid)
	$fgClass = [WindowFocusProbe]::ClassOf($fg)
	$owner = (Get-Process -Id $fgPid -ErrorAction SilentlyContinue).ProcessName
	$w = $p.MainWindowHandle
	$isFg = 0; $isHung = 0; $isMin = 0; $isAbove = 0
	if ($fg -eq [IntPtr]::Zero) { $noFg++ }
	$isShell = $shellOwners -contains $owner
	if ($isShell) { $shellFg++ }
	if ($w -ne [IntPtr]::Zero) {
		$seen++
		if ($fg -eq $w) { $isFg = 1; $engineFg++ }
		if ([WindowFocusProbe]::IsHungAppWindow($w)) { $isHung = 1; $hung++ }
		if ([WindowFocusProbe]::IsIconic($w)) { $isMin = 1; $minimized++ }
		if ($isFg -eq 0 -and -not $isShell -and $fg -ne [IntPtr]::Zero -and [WindowFocusProbe]::IsAbove($w, $fg)) { $isAbove = 1; $above++ }
	}
	if ($fgClass -eq "Ghost") { $ghostFg++ }
	$n++
	$rows.Add(("{0},{1},{2},{3},{4},{5},{6},{7},{8}" -f (Get-Date -Format HH:mm:ss.fff), $fg, $owner, $fgClass, $w, $isFg, $isHung, $isMin, $isAbove))
	Start-Sleep -Milliseconds $IntervalMs
}
if ($Csv) { $rows | Out-File -Encoding utf8 $Csv }
Write-Output ("samples={0} window_seen={1} engine_foreground={2} ghost_foreground={3} hung={4} minimized={5} above_foreground={6} shell_foreground={7} no_foreground={8}" -f $n, $seen, $engineFg, $ghostFg, $hung, $minimized, $above, $shellFg, $noFg)
if ($exited) { Write-Output "VERDICT background: EXITED"; exit 1 }
$pass = ($seen -gt 0) -and ($engineFg -eq 0) -and ($ghostFg -eq 0) -and ($minimized -eq 0) -and ($above -eq 0)
Write-Output ("VERDICT background: " + $(if ($pass) { "PASS" } else { "FAIL" }))
