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
#   Background: the z-order is the front-to-back stacking of top-level windows. The
#   active window is the one with keyboard focus; Windows normally raises a window when
#   it becomes active, and only then. Showing a window "without activation" therefore
#   leaves it on top while the user's window keeps the focus, and clicking that already
#   active window changes nothing, so the game would stay in front.
#
# USAGES
#   Started by run-pbr-ibl-smoke.sh in background window mode (the default), hidden
#   (-WindowStyle Hidden), with the game's Windows process id and
#   -LogFile <write-dir>/window-to-back.log. It is never an entry point by itself.
#   By hand (PowerShell or Git Bash), with the Windows id shown in Task Manager:
#     powershell -NoProfile -ExecutionPolicy Bypass -File pbr-window-to-back.ps1 -ProcessId <pid>
#   Options:
#     -ProcessId N   required; Windows process id (not a Git Bash/MSYS id: the launcher
#                    reads the Windows id from /proc/<msys pid>/winpid)
#     -Seconds N     how long to keep watching, default 900 (15 minutes, longer than a
#                    Debug self-test); afterwards the window is left where it is
#     -LogFile PATH  optional; one line when the window is first found (and again if
#                    the process's main window handle changes) and one when the helper ends
#
# HOW TO USE (expected output)
#   The helper prints nothing to the console (the launcher runs it hidden). With
#   -LogFile the log reads, for example (a real self-test run):
#     18:21:21.930 keeping window 6952892 at the bottom
#   The "done after N bottom placements" line is written only when the helper ends by
#   itself (game exited between two passes, or -Seconds reached). Under the launcher it
#   is usually missing: the launcher stops the helper as soon as the engine exits.
#   No "keeping window" line means the process never showed a main window (wrong id, or
#   the game failed before opening its window; check infolog.txt). Out-File -Encoding
#   utf8 in Windows PowerShell 5.1 starts the file with a byte-order mark.
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
#   Worked example, keep the game directly behind VS Code instead of at the bottom:
#     $after = (Get-Process Code | Where-Object { $_.MainWindowHandle -ne 0 } |
#               Select-Object -First 1).MainWindowHandle
#     [void][PbrWindowToBack]::SetWindowPos($hwnd, $after, 0, 0, 0, 0, $flags)
#   (SetWindowPos places the window just below the window passed as "after".) Add a new
#   Win32 call by declaring it in the Add-Type block below, with the constants it needs
#   named there as well, never as bare numbers in the loop.
param(
	[Parameter(Mandatory = $true)][int]$ProcessId,
	[int]$Seconds = 900,
	[string]$LogFile = ""
)

# PowerShell cannot call user32.dll directly; Add-Type compiles this small C# class
# with a P/Invoke declaration of SetWindowPos and the Win32 constants it uses (values
# from WinUser.h): HWND_BOTTOM ((HWND)1) means "behind every other window",
# SWP_NOSIZE and SWP_NOMOVE keep the size and position, SWP_NOACTIVATE keeps the focus
# where it is. The class name is unique so it cannot clash with another loaded type.
Add-Type @"
using System;
using System.Runtime.InteropServices;
public static class PbrWindowToBack {
	[DllImport("user32.dll")] public static extern bool SetWindowPos(IntPtr hWnd, IntPtr after, int x, int y, int cx, int cy, uint flags);
	public static readonly IntPtr HWND_BOTTOM = new IntPtr(1);
	public const uint SWP_NOSIZE = 0x0001, SWP_NOMOVE = 0x0002, SWP_NOACTIVATE = 0x0010;
}
"@

# State for the loop: the combined flags, the deadline, how many placements were made
# (for the log) and the handle last seen (so the log names each new handle once).
$flags = [PbrWindowToBack]::SWP_NOSIZE -bor [PbrWindowToBack]::SWP_NOMOVE -bor [PbrWindowToBack]::SWP_NOACTIVATE
$end = (Get-Date).AddSeconds($Seconds)
$moves = 0
$found = [IntPtr]::Zero
# Once a second: stop when the game has exited; otherwise, as soon as the process has a
# main window (none while the engine is still starting), push it to the bottom again.
# Repeating the call is what keeps it there: any later "show" or a click on the game's
# taskbar button may raise it, and the next pass moves it back. One second is short
# enough that the window never stays in front noticeably, and the call is cheap.
# MainWindowHandle is re-read every pass because Get-Process returns a fresh object.
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
# Final log line when the helper ends by itself (see HOW TO USE: the launcher usually
# stops it first, so this line is often absent).
if ($LogFile) { "$(Get-Date -Format HH:mm:ss.fff) done after $moves bottom placements" | Out-File -Append -Encoding utf8 $LogFile }
