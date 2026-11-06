+++
title = 'Background Window'
date = 2026-11-03T00:00:00+00:00
draft = false
+++

## Overview

Automated tests, CI jobs, replay runs and captures often start the engine on a machine someone is using.
They need rendering, so the [headless build](../headless-and-dedicated) does not fit, but the window gets in the way:

 * it **takes the keyboard focus** when it opens, so keys typed into another window go to the game;
 * it **opens on top** of the other windows and covers them;
 * on Windows, a start-up that stops answering for about 5 s (a long game load does) is replaced by a **"Not Responding" ghost window**, which Windows activates, so it takes the focus as well;
 * it **can be minimized** (`MinimizeOnFocusLoss` in fullscreen, or `Spring.SetWindowMinimized`).

The `BackgroundWindow` config key, or the `--background-window` switch, turns all of that off.

## Usage

For one run, without changing any saved setting:

```bash
./spring --background-window
```

For every run that uses a given `springsettings.cfg`:

```
BackgroundWindow = 1
```

The switch sets the key for that run only; nothing is written to `springsettings.cfg`.
Set it before the engine starts: the window is created according to it.

## What it does

| Behaviour | How |
|---|---|
| does not take the focus when the window opens | the window is created hidden and shown without activation (`SDL_HINT_WINDOW_NO_ACTIVATION_WHEN_SHOWN`); on X11 `_NET_WM_USER_TIME` is also set to 0 before it is shown, the EWMH way to ask the window manager not to focus it |
| opens behind the other windows | it is lowered before and after it is shown, and whenever the engine changes the window's size, position or border (`SetWindowPos(HWND_BOTTOM)` on Windows; on X11 the EWMH `_NET_RESTACK_WINDOW` request, which mutter needs, and `XLowerWindow`) |
| is not replaced by a ghost window | Windows ghosting is turned off for the process (`DisableProcessWindowsGhosting`) |
| never minimizes or maximizes itself | `MinimizeOnFocusLoss` is ignored, and `Spring.SetWindowMinimized` and `Spring.SetWindowMaximized` do nothing and return `false` (maximizing activates the window) |
| never goes fullscreen | `Fullscreen` is ignored, also when a game asks for fullscreen later (`Spring.SetWindowGeometry`); the saved value is kept. The window is not maximized when its size equals the desktop's, because maximizing activates it |
| does not move the user's pointer | while the window has no focus, `Spring.WarpMouse` and the engine's own mouse warps leave the system pointer where it is; the engine still uses the new position, so `Spring.GetMouseState` reports it. Input grabbing (`grabinput`) already applies only while the window has the focus |

The window stays behind your windows until you click it; from then on it behaves like any window.
The infolog says `background window: shown without activation, lowered, never minimized`.

Tested on Windows 11 and on Linux X11 with the openbox, xfwm4 (Xfce), KWin (KDE) and mutter (GNOME) window managers; other X11 window managers should behave the same where they follow EWMH.
Not covered: macOS (neither the lowering nor the X11 focus request exists there), focus and stacking in Wayland sessions where SDL uses its own Wayland driver instead of X11 (the switch starts and runs there, but the compositor decides), a crash or assertion message box (it still takes the focus), and minimizing the window by hand.

## Testing

`tools/background-window-test/` has a runner and a probe for Windows (`run-test.ps1`, `window-focus-probe.ps1`) and for Linux X11 (`run-test.sh`, `window-focus-probe.sh`, which need `xdotool` and `xprop`).
The runner starts the engine once per case, without a game or map, so the engine opens its built-in menu; the probe samples the window five times a second without sending any input:

```bash
# Windows, PowerShell
powershell -NoProfile -ExecutionPolicy Bypass -File tools\background-window-test\run-test.ps1 -Engine C:\path\to\spring.exe
# Linux
tools/background-window-test/run-test.sh /path/to/spring
```

Start it from the terminal that has the focus and keep it focused; do not click the engine windows.

| Case | Engine started with | Passes when |
|---|---|---|
| `off` | nothing new | not judged: the "before" result, which takes the focus (Windows only lets it do so when the runner itself had the focus) |
| `switch` | `--background-window` | no sample with the engine focused, minimized, above the focused window or replaced by a ghost, and the infolog line is there |
| `fullscreen` | `--background-window`, `Fullscreen = 1` | as `switch`, the infolog shows only `windowed::` display modes, and `Fullscreen = 0` was not saved |
| `config` | `BackgroundWindow = 1` in `springsettings.cfg` | as `switch` |

The probe's last line is `VERDICT background:` `PASS`, `FAIL`, `EXITED` (the engine ended during the run) or, on Linux, `PARTIAL` (the window manager publishes no stacking list, so "above" was not measured).
Each case leaves its write directory, infolog and per-sample CSV in the output folder the runner prints.
