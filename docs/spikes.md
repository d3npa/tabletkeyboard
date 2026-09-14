# P0 feasibility spikes — results

Where each item was actually executed. The target device (Let's Note CF-QV,
KDE Plasma 5 on X11, fcitx5-mozc) is not reachable from this development
machine, so IME and tray items are marked **pending** and must be re-run there.

Harness: `Xvfb :99` (1600x1000x24, XTEST + XKB), optional `fluxbox` as a real
window manager, `xev` as the receiving application, and a throwaway 60-line
XTEST driver for clicks/drags (`/tmp`, not part of the repository).

## S1 — IME path through XTEST — **pending on target**

Prerequisites proven here instead: injected events arrive in the focused
application as ordinary events (`synthetic NO` in `xev`), with the expected
keysym and modifier state:

```
KeyPress   keycode 10 (keysym 0x31, 1)            state 0x100
KeyPress   keycode 37 (keysym 0xffe3, Control_L)  state 0x100
KeyPress   keycode 54 (keysym 0x63, c)            state 0x104   → XLookupString gives 0x03 (ETX)
KeyPress   keycode 8  (keysym 0xff2a, Zenkaku_Hankaku)
```

To finish on the target: inject romaji into Konsole/Kate/Firefox/xterm with
fcitx5-mozc active and compare composition, 変換/無変換 and 半角/全角 with the
physical keyboard (see `test-matrix.md`).

## S2 — keycode strategy — **verified on Xvfb**

| Question | Result |
|---|---|
| Natural keycodes for ordinary keys | `1` → keycode 10, `c` → keycode 54, `Return`, `BackSpace`, arrows, numpad: all natural |
| Natural keycodes for JP keysyms (on a US map) | `Henkan` → keycode 100, `Hiragana_Katakana` → keycode 101 (both present in the stock map) |
| Keysyms absent from the map | `Zenkaku_Hankaku`, `Eisu_toggle` → allocated unused keycodes (8, 97), all levels set to the same keysym; server stores 4 levels per key |
| Warm reuse | a second tap of the same key reuses the same keycode (no remap churn) |
| Immunity to modifier state | sticky Shift held + spare key → keysym unchanged (`state 0x101`, keysym `Zenkaku_Hankaku`) |
| Restore on exit | `xmodmap -pke` md5 identical before/after, including a SIGTERM exit (handlers installed for INT/TERM/HUP) |
| Map reset under us (`XkbMapNotify` from our own remap) | spares re-adopted instead of dropped (this was a real bug: without it the allocator forgot its spares and leaked keycodes) |
| Map reset by someone else (Plasma layout daemon) | spares that no longer hold our keysym are dropped; the pool is rebuilt from the fresh map |
| **Spare modifier keycodes** | **probe fails on this server**: after `XChangeKeyboardMapping` of an unused keycode to `Shift_L`, `XGetModifierMapping` does not list it under ShiftMask (XKB assigns modifiers from the compat rules, not from a raw keymap change). The backend therefore falls back to the *real* modifier keycodes — the fallback the plan already allowed. Practical consequence: OSK modifiers are pressed and released inside one script, so a physically held modifier is only at risk in the rare case of pressing the same modifier physically *while* an OSK chord is being injected. |

## S3 — focusless floating window — **verified on Xvfb (no WM) and fluxbox**

| Check | Result |
|---|---|
| `WM_HINTS.input` | `False` |
| Taskbar/pager | `_NET_WM_STATE ... SKIP_TASKBAR, SKIP_PAGER`; window type `_NET_WM_WINDOW_TYPE_UTILITY` |
| Always on top | `_NET_WM_STATE_ABOVE, STAYS_ON_TOP` |
| All desktops | `_NET_WM_DESKTOP = 0xFFFFFFFF`; **fluxbox additionally set `_NET_WM_STATE_STICKY`** from it |
| Focus stays put | `XGetInputFocus` returned the `xev` window before and after clicking OSK keys, with and without a WM |
| Injected keys land in the focused window | yes (all observations above are `xev` output while the pointer was over the keyboard) |
| `startSystemMove()` drag | works under fluxbox (`_NET_WM_MOVERESIZE`); under Xvfb without a WM the manual fallback takes over |
| Position memory | saved on hide/quit, restored per screen, clamped back on-screen |

## S4 — tray + single instance — **partially verified**

- Single instance: a second `tabletkeyboard --mode simple --lang jp106`
  forwarded its arguments to the running instance and exited 0; the running
  keyboard switched mode and language and resized.
- Tray: **pending on target.** Xvfb has no SNI host or XEmbed tray, so
  `QSystemTrayIcon::isSystemTrayAvailable()` is false; the app detects this and
  shows the window at startup instead of becoming unreachable.
  Check on the target: `busctl --user tree org.kde.StatusNotifierWatcher`.

## S5 — touch plumbing — **pending on target**

Widgets do not accept `QTouchEvent`, so Qt synthesizes ordinary mouse events
from single touches — exactly what `KeyButton` consumes. Multi-touch chords are
not part of v1 (see P6 in the plan), so no duplicate synthetic presses are
possible. Verify on the device: taps produce one press/release pair and do not
move the X input focus (same check as S3).

## Extra checks performed

- Idle CPU ≈ 1 % of one core, no polling loops (X connection events go through a
  `QSocketNotifier`); repaints are state-driven.
- `us` full mode renders correctly (visual check of a screenshot: F-row, five
  main rows, nav cluster, numpad, title bar with mode/language/hide).
- `jp106` renders JP labels (半/全, 英数, 変換, 無変換, かな) without tofu
  (`fc-list :lang=ja` → 88 fonts on this machine).
- `--check-layout` passes for both bundled layouts and fails for unknown
  keysyms/actions (unit tests).
