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
# Normally started by:  tools/scripts/run-pbr-ibl-smoke.sh --input-probe
# Results: <write-dir>/pbr-ibl-input-probe/report.txt plus step screenshots.
#
# Implementation notes: SDL2 reads scan codes, so every key event carries the
# MapVirtualKey scan code (arrow keys also KEYEVENTF_EXTENDEDKEY); chat text is
# sent as Unicode SendInput events. Desktop capture of the borderless OpenGL
# window can be stale, so screenshots are taken by the engine (F12).
param(
	[Parameter(Mandatory = $true)][string]$WriteDirectory,
	[int]$TargetProcessId = 0,
	[int]$LoadTimeoutSeconds = 300
)
$ErrorActionPreference = "Stop"
$Infolog = Join-Path $WriteDirectory "infolog.txt"
$LayoutFile = Join-Path (Join-Path $WriteDirectory "pbr-ibl-viewer") "layout.txt"
$ShotDir = Join-Path $WriteDirectory "screenshots"
$OutDir = Join-Path $WriteDirectory "pbr-ibl-input-probe"
$ViewerName = "PBR IBL and Cubemap Viewer"
if (Test-Path $OutDir) { Remove-Item -Recurse -Force $OutDir }
New-Item -ItemType Directory -Force $OutDir | Out-Null
New-Item -ItemType Directory -Force $ShotDir | Out-Null
Add-Type -AssemblyName System.Drawing
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
$extendedKeys = @(0x21, 0x22, 0x25, 0x26, 0x27, 0x28)
function Key($vk, $holdMs = 60) {
	$scan = [byte][PbrProbe]::MapVirtualKey([uint32]$vk, 0)
	$flags = 0; if ($extendedKeys -contains $vk) { $flags = 1 }
	[PbrProbe]::keybd_event([byte]$vk, $scan, $flags, [UIntPtr]::Zero); Start-Sleep -Milliseconds $holdMs
	[PbrProbe]::keybd_event([byte]$vk, $scan, ($flags -bor 2), [UIntPtr]::Zero); Start-Sleep -Milliseconds 200
}
function TypeCommand($text) {
	Key 0x0D
	Start-Sleep -Milliseconds 400
	foreach ($c in $text.ToCharArray()) { [PbrProbe]::Unicode($c); Start-Sleep -Milliseconds 15 }
	Start-Sleep -Milliseconds 250
	Key 0x0D
	Start-Sleep -Milliseconds 900
}
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
function Center($r) { return @([int](($r[0] + $r[2]) / 2), [int](($r[1] + $r[3]) / 2)) }

# Mean absolute RGB difference of the world area right of the panel (sampled).
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

try {
	# remember whether the viewer was already enabled so it can be left that way
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
	$r = $layout.controls["resource"]; $rowH = $r[3] - $r[1]; $c = Center $r
	Click $c[0] $c[1]
	# the viewer opens a list downward when it fits above the screen bottom,
	# otherwise upward (same rule as DropdownRows in the viewer)
	if ($r[1] - 6 * $rowH -ge 0) { $optionY = $r[1] - 6 * $rowH + [int]($rowH / 2) } else { $optionY = $r[3] + [int]($rowH / 2) }
	Click $c[0] $optionY
	# 3. focused: Up twice must step the mip; capture "before" with the panel still focused
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
finally {
	if (-not $viewerWasEnabled) { TypeCommand "/luaui disablewidget $ViewerName" }
	[PbrProbe]::SetCursorPos($originalCursor.X, $originalCursor.Y) | Out-Null
	$report.Add(("SUMMARY pass={0} fail={1}" -f $counts.PASS, $counts.FAIL))
	$report | Set-Content -Encoding utf8 (Join-Path $OutDir "report.txt")
	Write-Host ("SUMMARY pass={0} fail={1}" -f $counts.PASS, $counts.FAIL)
	TypeCommand "/quitforce"
}
if ($counts.FAIL -gt 0) { exit 1 }
