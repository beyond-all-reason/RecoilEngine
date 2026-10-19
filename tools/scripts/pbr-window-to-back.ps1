# Keeps an unattended test's game window at the bottom of the window z-order.
#
# WHY
#   run-pbr-ibl-smoke.sh --window-mode background shows the game window without
#   activating it (SDL_WINDOW_NO_ACTIVATION_WHEN_SHOWN). Windows still inserts such a
#   window at the top of the z-order, and clicking the already-active window behind
#   it (for example the user's editor) does not raise that window again. This helper
#   moves the game window to the bottom without activating it, so it never covers the
#   user's windows. The window keeps rendering (desktop composition), so the self-test
#   is unaffected. It never minimizes the window: Recoil nearly stops drawing then.
#
# USAGES
#   Started by run-pbr-ibl-smoke.sh in background window mode, hidden, with the game's
#   process id. By hand:
#     powershell -NoProfile -ExecutionPolicy Bypass -File pbr-window-to-back.ps1 -ProcessId <pid>
#   Options: -Seconds N (how long to keep watching, default 900), -LogFile PATH
#   (optional; see below for what it records).
#
# HOW IT WORKS / HOW TO EXTEND
#   Every second it looks up the process's main window and calls
#   SetWindowPos(HWND_BOTTOM) with SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE. Desktop
#   windows always stay below it, so the call is simply repeated rather than skipped
#   when the window "is already last"; it never activates or redraws anything. The log
#   gets one line when the window is first found and one when the helper ends. It exits
#   when the process exits or the time is up.
#   To keep the window somewhere else in the z-order instead (for example
#   directly behind one chosen window), replace HWND_BOTTOM by that window's handle.
param(
	[Parameter(Mandatory = $true)][int]$ProcessId,
	[int]$Seconds = 900,
	[string]$LogFile = ""
)

Add-Type @"
using System;
using System.Runtime.InteropServices;
public static class PbrWindowToBack {
	[DllImport("user32.dll")] public static extern bool SetWindowPos(IntPtr hWnd, IntPtr after, int x, int y, int cx, int cy, uint flags);
	public static readonly IntPtr HWND_BOTTOM = new IntPtr(1);
	public const uint SWP_NOSIZE = 0x0001, SWP_NOMOVE = 0x0002, SWP_NOACTIVATE = 0x0010;
}
"@

$flags = [PbrWindowToBack]::SWP_NOSIZE -bor [PbrWindowToBack]::SWP_NOMOVE -bor [PbrWindowToBack]::SWP_NOACTIVATE
$end = (Get-Date).AddSeconds($Seconds)
$moves = 0
$found = [IntPtr]::Zero
while ((Get-Date) -lt $end) {
	$process = Get-Process -Id $ProcessId -ErrorAction SilentlyContinue
	if (-not $process) { break }
	$hwnd = $process.MainWindowHandle
	if ($hwnd -ne [IntPtr]::Zero) {
		if ($hwnd -ne $found -and $LogFile) { "$(Get-Date -Format HH:mm:ss.fff) keeping window $hwnd at the bottom" | Out-File -Append -Encoding utf8 $LogFile }
		$found = $hwnd
		[void][PbrWindowToBack]::SetWindowPos($hwnd, [PbrWindowToBack]::HWND_BOTTOM, 0, 0, 0, 0, $flags)
		$moves++
	}
	Start-Sleep -Seconds 1
}
if ($LogFile) { "$(Get-Date -Format HH:mm:ss.fff) done after $moves bottom placements" | Out-File -Append -Encoding utf8 $LogFile }
