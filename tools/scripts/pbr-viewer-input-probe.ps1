# Real Windows input probe for BAR's "PBR IBL and Cubemap Viewer".
#
# Drives an already running isolated PBR runtime with genuine operating-system
# mouse and keyboard input (the same path a person uses) and checks, from
# engine screenshots and the viewer's /pbribllayout log, that:
#   * typing "/luaui enablewidget PBR IBL and Cubemap Viewer" in chat opens it;
#   * clicking a dropdown and an option changes the viewer;
#   * while the panel is focused, keys change the viewer and do NOT reach the
#     game (no camera pan, F12 screenshot and Enter chat swallowed);
#   * after Esc the same key reaches the game again (camera pans);
#   * the mouse wheel over the panel steps the mip without zooming the camera,
#     while the wheel outside the panel zooms.
#
# WARNING: this moves the real mouse cursor and sends real key presses to the
# foreground window for about a minute. Do not use the computer meanwhile.
#
# WHY
#   BAR's self-test injects viewer call-ins directly, which proves the viewer's
#   logic but not that genuine Windows input travels SDL -> Recoil -> LuaUI ->
#   the viewer the way a person's does, nor that a focused panel really keeps
#   keys and the wheel away from the game camera. Only real operating-system
#   input can show that, so this probe exists as a separate, opt-in check.
#
# SAFETY
#   Windows delivers synthetic input (SendInput, keybd_event, mouse_event) to
#   whichever window is in front and to whatever lies under the cursor, not to
#   a chosen window. The probe brings the game to the front once; if another
#   window comes to the front during the run, the remaining keys and clicks go
#   to that window. Therefore: run it only while away from the computer, never
#   in the launcher's background window mode (the launcher refuses that), and
#   expect a typed chat line such as "/quitforce" to land elsewhere if the game
#   lost the front. The finally block always restores the cursor position,
#   disables the viewer again if it was off, writes the report and quits the game.
#
# USAGES
#   Normally started by:  tools/scripts/run-pbr-ibl-smoke.sh --input-probe
#                                                      --window-mode foreground
#   which launches the engine itself and passes -WriteDirectory and
#   -TargetProcessId. By hand, against a game already running isolated:
#     powershell -NoProfile -ExecutionPolicy Bypass -File pbr-viewer-input-probe.ps1
#       -WriteDirectory C:\...\test-results\pbr-ibl [-TargetProcessId <windows pid>]
#       [-LoadTimeoutSeconds 300]
#   Parameters:
#     -WriteDirectory      the engine's --write-dir (infolog.txt, screenshots/,
#                          pbr-ibl-viewer/layout.txt live there); required
#     -TargetProcessId     Windows process id of the engine; 0 (default) means
#                          the first process named "spring" with a window
#     -LoadTimeoutSeconds  how long to wait for the window and for the
#                          "finished loading" infolog line (default 300)
#   Needs: Windows PowerShell 5.1 (64-bit), the isolated config's LogFlush = 1
#   (the launcher sets it), and BAR's viewer widget with its /pbribllayout command.
#   Exit code 0 = every check passed, 1 = a FAIL line (or a thrown error).
#
# HOW TO USE (expected output)
#   Console and report.txt list the same lines (a real 2026-10-15 run):
#     INFO game window 2560x1440 at 0,0; foreground=True
#     INFO viewer initially enabled: True
#     PASS typed chat command opens the viewer | /luaui enablewidget PBR IBL and Cubemap Viewer
#     PASS F12 does not reach the engine while the panel is focused | engine screenshots 43 -> 43
#     PASS real clicks select a dropdown option and focused Up keys step the mip | resource=pbr-specular mip=2
#     PASS Esc releases keyboard focus | focused=false
#     PASS a held arrow key does not move the camera while the panel is focused | world-area difference 0.19
#     PASS the same arrow key reaches the game once focus is released | world-area difference 28.75
#     PASS the wheel over the panel steps the mip and does not zoom the camera | world-area difference 0.00; mip 3 -> 2
#     PASS the wheel outside the panel still zooms the camera | world-area difference 14.87
#     SUMMARY pass=8 fail=0
#   (the console lines carry a [HH:mm:ss] time for INFO lines; the report adds the
#   INFO layout ... lines of each /pbribllayout request). The six step screenshots
#   01_..06_*.png are copied next to the report.
#
# HOW TO EXTEND (worked example: one more key check)
#   To prove that PageDown steps the mip while the panel is focused, add after
#   step 3, using the helpers below (VK_NEXT = 0x22 is already in $extendedKeys):
#     $c = Center $layout.controls["mip-value"]; Click $c[0] $c[1]   # focus panel
#     Key 0x22                                                      # VK_NEXT
#     Key 0x1B                                                      # Esc: release
#     $after = Layout
#     Check ($after.state.mip -eq ...expected...) "PageDown steps the mip" "mip=$($after.state.mip)"
#   Read state only through Layout (the viewer's file answer), never by guessing
#   from screen pixels; use Shot + WorldDiff only for "did the camera move".
#   Every Check line becomes a PASS/FAIL line and counts towards the exit code.
#
# Implementation notes: SDL2 reads scan codes, so every key event carries the
# MapVirtualKey scan code (arrow keys also KEYEVENTF_EXTENDEDKEY); chat text is
# sent as Unicode SendInput events. Desktop capture of the OpenGL window can be
# stale, so screenshots are taken by the engine (F12).
param(
	[Parameter(Mandatory = $true)][string]$WriteDirectory,
	[int]$TargetProcessId = 0,
	[int]$LoadTimeoutSeconds = 300
)
# Stop on the first error so a failed step reaches the catch block as a FAIL
# instead of continuing with stale state.
$ErrorActionPreference = "Stop"
# Files the engine and the viewer write: infolog.txt (loading finished),
# pbr-ibl-viewer/layout.txt (answers to /pbribllayout), screenshots/ (F12).
# The output folder is emptied first so a report never mixes two runs.
$Infolog = Join-Path $WriteDirectory "infolog.txt"
$LayoutFile = Join-Path (Join-Path $WriteDirectory "pbr-ibl-viewer") "layout.txt"
$ShotDir = Join-Path $WriteDirectory "screenshots"
$OutDir = Join-Path $WriteDirectory "pbr-ibl-input-probe"
$ViewerName = "PBR IBL and Cubemap Viewer"
if (Test-Path $OutDir) { Remove-Item -Recurse -Force $OutDir }
New-Item -ItemType Directory -Force $OutDir | Out-Null
New-Item -ItemType Directory -Force $ShotDir | Out-Null
# System.Drawing reads the engine screenshots for WorldDiff.
Add-Type -AssemblyName System.Drawing
# Win32 calls through a small C# class (PowerShell cannot call user32.dll
# directly). Raw values in this block and below come from WinUser.h:
#   INPUT.type 1 = INPUT_KEYBOARD; KEYBDINPUT.dwFlags 4 = KEYEVENTF_UNICODE,
#   6 = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP (2); keybd_event flags 1 =
#   KEYEVENTF_EXTENDEDKEY, 2 = KEYEVENTF_KEYUP; MapVirtualKey type 0 =
#   MAPVK_VK_TO_VSC (virtual key to scan code); mouse_event 0x02/0x04 =
#   MOUSEEVENTF_LEFTDOWN/LEFTUP, 0x0800 = MOUSEEVENTF_WHEEL (data in multiples
#   of WHEEL_DELTA = 120 per notch, negative = rotated towards the user).
#   Virtual keys: 0x0D Enter, 0x12 Alt (scan 0x38), 0x1B Esc, 0x21/0x22
#   PageUp/PageDown, 0x25-0x28 Left/Up/Right/Down, 0x7B F12.
# INPUT is declared with an explicit 40-byte size and the KEYBDINPUT at offset
# 8: that is the 64-bit layout (the union after a 4-byte type is 8-aligned).
# A 32-bit PowerShell would need size 28 and offset 4, so run the 64-bit one.
# Unicode(c) types one character as a key-down/key-up pair carrying the
# character itself, so the chat receives text independent of keyboard layout.
Add-Type @"
using System;
using System.Runtime.InteropServices;
public static class PbrProbe {
  [StructLayout(LayoutKind.Sequential)] public struct POINT { public int X, Y; }
  [StructLayout(LayoutKind.Sequential)] public struct RECT { public int L, T, R, B; }
  [StructLayout(LayoutKind.Sequential)] public struct KEYBDINPUT { public ushort wVk; public ushort wScan; public uint dwFlags; public uint time; public IntPtr dwExtraInfo; }
  [StructLayout(LayoutKind.Explicit, Size = 40)] public struct INPUT { [FieldOffset(0)] public uint type; [FieldOffset(8)] public KEYBDINPUT ki; }
  [DllImport("user32.dll")] public static extern uint SendInput(uint n, INPUT[] inputs, int size);
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
  [DllImport("user32.dll")] public static extern IntPtr GetForegroundWindow();
  [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr h, out RECT r);
  [DllImport("user32.dll")] public static extern bool ClientToScreen(IntPtr h, ref POINT p);
  [DllImport("user32.dll")] public static extern bool SetCursorPos(int x, int y);
  [DllImport("user32.dll")] public static extern bool GetCursorPos(out POINT p);
  [DllImport("user32.dll")] public static extern void mouse_event(uint f, int dx, int dy, int data, UIntPtr e);
  [DllImport("user32.dll")] public static extern void keybd_event(byte vk, byte scan, uint f, UIntPtr e);
  [DllImport("user32.dll")] public static extern uint MapVirtualKey(uint code, uint type);
  public static void Unicode(char c) {
    INPUT[] a = new INPUT[2];
    a[0].type = 1; a[0].ki.wScan = c; a[0].ki.dwFlags = 4;
    a[1].type = 1; a[1].ki.wScan = c; a[1].ki.dwFlags = 6;
    SendInput(2, a, Marshal.SizeOf(typeof(INPUT)));
  }
}
"@

# Report lines are collected in memory and written once in the finally block.
# Note prints and records an INFO line; Check records a PASS or FAIL line and
# counts it (the counts make the SUMMARY line and the exit code).
$report = New-Object System.Collections.Generic.List[string]
$counts = @{ PASS = 0; FAIL = 0 }
$viewerWasEnabled = $false
function Note($text) { $line = "[{0:HH:mm:ss}] {1}" -f (Get-Date), $text; Write-Host $line; $report.Add("INFO $text") }
function Check($ok, $name, $detail) {
	$kind = if ($ok) { "PASS" } else { "FAIL" }
	$counts[$kind]++
	$line = "$kind $name | $detail"
	Write-Host $line
	$report.Add($line)
}

# --- wait for the game ---------------------------------------------------
# First the process must have a main window (polled every 0.5 s, hence the
# factor 2), then infolog.txt must report that loading finished (polled every
# second). The extra 6 s let the first in-game frames and widgets settle
# before any input is sent. If loading never finishes, the probe continues
# after the timeout and the checks will fail visibly.
$proc = $null
for ($i = 0; $i -lt ($LoadTimeoutSeconds * 2) -and -not $proc; $i++) {
	if ($TargetProcessId -gt 0) {
		$proc = Get-Process -Id $TargetProcessId -ErrorAction SilentlyContinue | Where-Object { $_.MainWindowHandle -ne 0 }
	} else {
		$proc = Get-Process spring -ErrorAction SilentlyContinue | Where-Object { $_.MainWindowHandle -ne 0 } | Select-Object -First 1
	}
	if (-not $proc) { Start-Sleep -Milliseconds 500 }
}
if (-not $proc) { throw "no target engine window appeared" }
for ($i = 0; $i -lt $LoadTimeoutSeconds; $i++) {
	if ((Test-Path $Infolog) -and (Select-String -Path $Infolog -Pattern "finished loading and is now ingame" -Quiet)) { break }
	Start-Sleep -Seconds 1
}
Start-Sleep -Seconds 6
# Bring the game to the front. Windows lets a program call SetForegroundWindow
# only if it received the last input event; tapping Alt (down, up) first makes
# this script that program, so the call is honoured instead of just flashing the
# taskbar button. The cursor position is saved to restore it at the end.
# The client rectangle (drawable area without title bar and borders) and its
# top-left corner in desktop pixels convert viewer coordinates to clicks.
$h = $proc.MainWindowHandle
$originalCursor = New-Object PbrProbe+POINT
[PbrProbe]::GetCursorPos([ref]$originalCursor) | Out-Null
[PbrProbe]::keybd_event(0x12, 0x38, 0, [UIntPtr]::Zero); [PbrProbe]::keybd_event(0x12, 0x38, 2, [UIntPtr]::Zero)
[PbrProbe]::SetForegroundWindow($h) | Out-Null
Start-Sleep -Milliseconds 500
$rect = New-Object PbrProbe+RECT; [PbrProbe]::GetClientRect($h, [ref]$rect) | Out-Null
$origin = New-Object PbrProbe+POINT; [PbrProbe]::ClientToScreen($h, [ref]$origin) | Out-Null
$clientW = $rect.R - $rect.L; $clientH = $rect.B - $rect.T
Note ("game window {0}x{1} at {2},{3}; foreground={4}" -f $clientW, $clientH, $origin.X, $origin.Y, ([PbrProbe]::GetForegroundWindow() -eq $h))

# --- input helpers -------------------------------------------------------
# Keys that need KEYEVENTF_EXTENDEDKEY (PageUp, PageDown and the arrows): without
# it Windows reports the numeric-keypad key with the same scan code instead.
# Key presses one virtual key with its scan code (SDL ignores events without a
# scan code), holds it $holdMs (a long hold lets the camera pan), releases it
# and waits 200 ms so the engine sees one frame per change.
$extendedKeys = @(0x21, 0x22, 0x25, 0x26, 0x27, 0x28)
function Key($vk, $holdMs = 60) {
	$scan = [byte][PbrProbe]::MapVirtualKey([uint32]$vk, 0)
	$flags = 0; if ($extendedKeys -contains $vk) { $flags = 1 }
	[PbrProbe]::keybd_event([byte]$vk, $scan, $flags, [UIntPtr]::Zero); Start-Sleep -Milliseconds $holdMs
	[PbrProbe]::keybd_event([byte]$vk, $scan, ($flags -bor 2), [UIntPtr]::Zero); Start-Sleep -Milliseconds 200
}
# Types a chat command as a person does: Enter opens the chat box, the text is
# typed character by character, Enter sends it. The pauses let the chat box open
# and the command run before the next step.
function TypeCommand($text) {
	Key 0x0D
	Start-Sleep -Milliseconds 400
	foreach ($c in $text.ToCharArray()) { [PbrProbe]::Unicode($c); Start-Sleep -Milliseconds 15 }
	Start-Sleep -Milliseconds 250
	Key 0x0D
	Start-Sleep -Milliseconds 900
}
# Engine screenshots: F12 makes Recoil write a PNG into <write-dir>/screenshots.
# ShotCount counts them (also used to prove F12 was swallowed). Shot presses
# F12, waits up to 40 x 250 ms = 10 s for a new file, waits 0.8 s more so the
# PNG is completely written, and copies it as <name>.png into the output folder;
# it returns $null when no screenshot appeared (WorldDiff then returns NaN and
# the check fails).
function ShotCount { return @(Get-ChildItem $ShotDir -Filter *.png -ErrorAction SilentlyContinue).Count }
function Shot($name) {
	$before = ShotCount
	Key 0x7B
	for ($i = 0; $i -lt 40; $i++) {
		Start-Sleep -Milliseconds 250
		$files = @(Get-ChildItem $ShotDir -Filter *.png | Sort-Object LastWriteTime)
		if ($files.Count -gt $before) {
			Start-Sleep -Milliseconds 800
			$target = Join-Path $OutDir "$name.png"
			Copy-Item $files[-1].FullName $target
			return $target
		}
	}
	return $null
}
# Lua screen coordinates (origin bottom-left) -> desktop pixels
# The viewer reports rectangles in Lua coordinates (y grows upwards from the
# bottom of the client area); Windows uses y growing downwards from the top of
# the desktop. MoveTo, Click and Wheel take Lua coordinates and move the real
# cursor there first, because Recoil reads the wheel and clicks at the cursor.
function ToDesktop($x, $y) { return @(($origin.X + [int]$x), ($origin.Y + $clientH - 1 - [int]$y)) }
function MoveTo($x, $y) { $p = ToDesktop $x $y; [PbrProbe]::SetCursorPos($p[0], $p[1]) | Out-Null; Start-Sleep -Milliseconds 200 }
function Click($x, $y) {
	MoveTo $x $y
	[PbrProbe]::mouse_event(0x02, 0, 0, 0, [UIntPtr]::Zero); Start-Sleep -Milliseconds 80
	[PbrProbe]::mouse_event(0x04, 0, 0, 0, [UIntPtr]::Zero); Start-Sleep -Milliseconds 500
}
function Wheel($x, $y, $delta) { MoveTo $x $y; [PbrProbe]::mouse_event(0x0800, 0, 0, $delta, [UIntPtr]::Zero); Start-Sleep -Milliseconds 700 }
$script:layoutCounter = 0
# tags are unique per run so a layout file left by an earlier run never matches
$script:runId = Get-Date -Format "HHmmss"
# Asks the viewer for its layout with a unique tag and waits until the
# viewer's layout file holds the answer to that request.
# The answer has one line per control ("control=<name> rect=x0,y0,x1,y1"), one
# for the panel ("panel=... focused=true|false") and one for the state
# ("state resource=<name> mip=<n> mode=<name>"). The file is used instead of
# reading infolog.txt because it holds only these answers. The request is typed
# through the chat, and a focused panel swallows the Enter that opens the chat,
# so callers release focus (Esc) before calling Layout.
function Layout($maxReads = 40) {
	$script:layoutCounter++
	$tag = "probe$($script:runId)-$($script:layoutCounter)"
	TypeCommand "/pbribllayout $tag"
	$result = @{ controls = @{}; state = @{}; panel = $null; focused = $null }
	for ($attempt = 0; $attempt -lt $maxReads -and $result.state.Count -eq 0; $attempt++) {
		Start-Sleep -Milliseconds 250
		if (-not (Test-Path $LayoutFile)) { continue }
		$text = (Get-Content -Raw $LayoutFile) -split "`n" | Where-Object { $_ -match "layout tag=$tag " }
		foreach ($l in $text) {
			if ($l -match " control=(\S+) rect=(-?\d+),(-?\d+),(-?\d+),(-?\d+)") { $result.controls[$matches[1]] = @([int]$matches[2], [int]$matches[3], [int]$matches[4], [int]$matches[5]) }
			if ($l -match " panel=(-?\d+),(-?\d+),(-?\d+),(-?\d+) focused=(\S+)") { $result.panel = @([int]$matches[1], [int]$matches[2], [int]$matches[3], [int]$matches[4]); $result.focused = $matches[5] }
			if ($l -match " state resource=(\S+) mip=(\d+) mode=(\S+)") { $result.state = @{ resource = $matches[1]; mip = [int]$matches[2]; mode = $matches[3] } }
		}
	}
	Note ("layout {0}: {1} controls, focused={2}, state={3}/{4} after {5} reads" -f $tag, $result.controls.Count, $result.focused, $result.state.resource, $result.state.mip, $attempt)
	return $result
}
# Center of a rectangle {x0, y0, x1, y1}, as integer Lua coordinates.
function Center($r) { return @([int](($r[0] + $r[2]) / 2), [int](($r[1] + $r[3]) / 2)) }

# Mean absolute RGB difference of the world area right of the panel (sampled).
# Used to tell "the camera moved" from "it did not": the area starts 60 px right
# of the panel (at most 200 px from the right edge) and spans 10 %-72 % of the
# height, chosen to leave out the panel and BAR's top and bottom UI; every 9th
# pixel in both directions is enough and keeps GetPixel fast. The result is in
# 0-255 units per channel. The checks below treat < 0.8 as "did not move" and
# > 3.0 as "moved" (measured: about 0.2 for a still camera, 15-29 for a pan or
# zoom); these thresholds are literals in the steps below.
function WorldDiff($a, $b, $panelRight) {
	if (-not $a -or -not $b) { return [double]::NaN }
	$ia = [System.Drawing.Bitmap]::FromFile($a); $ib = [System.Drawing.Bitmap]::FromFile($b)
	$x0 = [Math]::Min($ia.Width - 200, $panelRight + 60); $x1 = [int]($ia.Width * 0.9)
	$y0 = [int]($ia.Height * 0.10); $y1 = [int]($ia.Height * 0.72)
	$sum = 0.0; $n = 0
	for ($y = $y0; $y -lt $y1; $y += 9) {
		for ($x = $x0; $x -lt $x1; $x += 9) {
			$p = $ia.GetPixel($x, $y); $q = $ib.GetPixel($x, $y)
			$sum += [Math]::Abs($p.R - $q.R) + [Math]::Abs($p.G - $q.G) + [Math]::Abs($p.B - $q.B); $n += 3
		}
	}
	$ia.Dispose(); $ib.Dispose()
	return $sum / [Math]::Max(1, $n)
}

# --- the probe -----------------------------------------------------------
# Steps 1-5 run in order and each builds on the state left by the previous one.
# Any thrown error becomes one FAIL "probe completed" line; finally always runs.
try {
	# remember whether the viewer was already enabled so it can be left that way
	# (a short Layout: 8 reads of 250 ms; no answer means the viewer is off)
	$initially = Layout 8
	$viewerWasEnabled = ($initially.state.Count -gt 0)
	Note "viewer initially enabled: $viewerWasEnabled"

	# 1. open the viewer the documented way: typing the chat command
	TypeCommand "/luaui enablewidget $ViewerName"
	TypeCommand "/pbriblviewer show"           # visible and expanded, whatever the saved state
	Start-Sleep -Seconds 1
	$layout = Layout
	Check ($layout.controls.ContainsKey("resource")) "typed chat command opens the viewer" "/luaui enablewidget $ViewerName"
	if (-not $layout.controls.ContainsKey("resource")) { throw "viewer did not open" }
	$panelRight = $layout.panel[2]

	# 2. a real click opens the Resource dropdown; a click on its 6th row picks the GGX resource
	# (row 6 of the viewer's Resource list is "pbr-specular", the GGX cube; the
	# check in step 3 names it, so a reordered list fails there visibly)
	$r = $layout.controls["resource"]; $rowH = $r[3] - $r[1]; $c = Center $r
	Click $c[0] $c[1]
	# the viewer opens a list downward when it fits above the screen bottom,
	# otherwise upward (same rule as DropdownRows in the viewer)
	if ($r[1] - 6 * $rowH -ge 0) { $optionY = $r[1] - 6 * $rowH + [int]($rowH / 2) } else { $optionY = $r[3] + [int]($rowH / 2) }
	Click $c[0] $optionY
	# 3. focused: Up twice must step the mip; capture "before" with the panel still focused
	# The dropdown click left the panel focused. 0x26 = Up, 0x7B = F12,
	# 0x0D = Enter, 0x1B = Esc (see the Add-Type comment).
	Key 0x26; Key 0x26
	$shotsBefore = ShotCount
	Key 0x7B                                    # F12 must be swallowed while focused
	Key 0x0D                                    # Enter must not open the chat
	Start-Sleep -Milliseconds 1500
	Check ((ShotCount) -eq $shotsBefore) "F12 does not reach the engine while the panel is focused" "engine screenshots $shotsBefore -> $(ShotCount)"
	Key 0x1B                                    # release focus to read the state through chat
	Start-Sleep -Milliseconds 400
	$layout = Layout
	Check ($layout.state.resource -eq "pbr-specular" -and $layout.state.mip -eq 2) "real clicks select a dropdown option and focused Up keys step the mip" ("resource={0} mip={1}" -f $layout.state.resource, $layout.state.mip)
	Check ($layout.focused -eq "false") "Esc releases keyboard focus" "focused=$($layout.focused)"

	# 4. camera leak test: focused Up hold must not pan; unfocused Up hold must pan
	# The 2 s pauses let the camera come to rest before each screenshot, so the
	# difference measures only what the held key did.
	Start-Sleep -Seconds 2
	$s0 = Shot "01_before_focused_hold"
	$c = Center $layout.controls["mip-value"]
	Click $c[0] $c[1]                           # focus the panel (the value box has no action)
	Key 0x26 900                                # hold Up ~0.9 s while focused
	Key 0x1B
	Start-Sleep -Seconds 2
	$s1 = Shot "02_after_focused_hold"
	$dFocused = WorldDiff $s0 $s1 $panelRight
	Check ($dFocused -lt 0.8) "a held arrow key does not move the camera while the panel is focused" ("world-area difference {0:N2}" -f $dFocused)
	Key 0x26 900                                # unfocused: the camera should pan
	Start-Sleep -Seconds 2
	$s2 = Shot "03_after_unfocused_hold"
	$dUnfocused = WorldDiff $s1 $s2 $panelRight
	Check ($dUnfocused -gt 3.0) "the same arrow key reaches the game once focus is released" ("world-area difference {0:N2}" -f $dUnfocused)

	# 5. wheel: over the panel steps the mip only; outside the panel zooms
	# One notch (-120) over the face cross must lower the mip by one (not below
	# 0) and leave the world still; five notches (-600) far right of the panel
	# must zoom the camera.
	$layout = Layout
	$mipBefore = $layout.state.mip
	Start-Sleep -Seconds 2
	$s3 = Shot "04_before_wheel_over_panel"
	$c = Center $layout.controls["cross"]
	Wheel $c[0] $c[1] (-120)
	Start-Sleep -Seconds 2
	$s4 = Shot "05_after_wheel_over_panel"
	$dWheelPanel = WorldDiff $s3 $s4 $panelRight
	$layout = Layout
	Check ($dWheelPanel -lt 0.8 -and $layout.state.mip -eq [Math]::Max(0, $mipBefore - 1)) "the wheel over the panel steps the mip and does not zoom the camera" ("world-area difference {0:N2}; mip {1} -> {2}" -f $dWheelPanel, $mipBefore, $layout.state.mip)
	Wheel ($panelRight + 600) ([int]($clientH * 0.5)) (-600)
	Start-Sleep -Seconds 2
	$s5 = Shot "06_after_wheel_outside_panel"
	$dWheelOutside = WorldDiff $s4 $s5 $panelRight
	Check ($dWheelOutside -gt 3.0) "the wheel outside the panel still zooms the camera" ("world-area difference {0:N2}" -f $dWheelOutside)
	Note "screenshots of the panel drawn over BAR's UI: 01_before_focused_hold.png and others in this folder"
}
catch {
	Check $false "probe completed" $_.Exception.Message
}
# Clean-up runs even after a failure: leave the viewer as it was found, give the
# user the cursor back where it was, write the report, and quit the game so the
# launcher's wait returns. /quitforce quits without the confirmation dialog.
finally {
	if (-not $viewerWasEnabled) { TypeCommand "/luaui disablewidget $ViewerName" }
	[PbrProbe]::SetCursorPos($originalCursor.X, $originalCursor.Y) | Out-Null
	$report.Add(("SUMMARY pass={0} fail={1}" -f $counts.PASS, $counts.FAIL))
	$report | Set-Content -Encoding utf8 (Join-Path $OutDir "report.txt")
	Write-Host ("SUMMARY pass={0} fail={1}" -f $counts.PASS, $counts.FAIL)
	TypeCommand "/quitforce"
}
if ($counts.FAIL -gt 0) { exit 1 }
