# Feasibility spikes — results

Two harnesses were used:

- **Development machine** — `Xvfb :99` (1600x1000, XTEST + XKB), optionally
  `fluxbox` as a second WM, `xev` as the receiving application, and a throwaway
  60-line XTEST driver for clicks/drags (in `/tmp`, not part of the repository).
- **Target device** — Let's Note CF-QV: Slackware 15.0, Qt 5.15.3, KDE Plasma 5
  on X11 (`kwin_x11`), fcitx5 with `fcitx5-mozc`, `LANG=ja_JP.UTF-8`,
  `QT_IM_MODULE=fcitx`, `XMODIFIERS=@im=fcitx`, 2880x1920 panel. The program was
  built in `~/tabletkeyboard-test/src/build` (user-level, no installation) and
  driven over ssh; screenshots via `import` + `magick`.

## S1 — IME path through XTEST — **verified on the target**

Everything below was done purely with clicks on the on-screen keyboard, with
Kate focused and the user's existing fcitx5 configuration untouched:

| Step | Observation |
|---|---|
| 半角/全角 key | `fcitx5-remote` state `1` → `2` (IM activated); pressing it again deactivates |
| romaji `k a n j i` | mozc preedit `かんじ` with a candidate window (`感じる`, `感じ`, `患者`, …) |
| 変換 (`Henkan`) | converted; Enter confirmed → Kate buffer `感じ`, Ctrl+S wrote it to disk |
| sentence `watashihagakuseidesu` | 変換 + Enter → **`私は学生です`** saved by the OSK's Ctrl+S |
| かな + `aiueo` | hiragana `あいうえお` committed |
| 無変換 (`Muhenkan`) | deactivates the IME — exactly this user's binding: `~/.config/fcitx5/config` has `[Hotkey/ActivateKeys] 0=Henkan` and `[Hotkey/DeactivateKeys] 0=Muhenkan` |

The OSK drives the *existing* fcitx5 key bindings: no IPC, no plugin, no
configuration change (R6 holds on the real machine).

Prerequisite checks (dev machine, `xev`): injected events arrive as ordinary
events (`synthetic NO`) with the expected keysym and modifier state:

```
KeyPress   keycode 10 (keysym 0x31, 1)            state 0x100
KeyPress   keycode 37 (keysym 0xffe3, Control_L)  state 0x100
KeyPress   keycode 54 (keysym 0x63, c)            state 0x104   → XLookupString gives 0x03 (ETX)
KeyPress   keycode 8  (keysym 0xff2a, Zenkaku_Hankaku)   (spare keycode on Xvfb)
```

## S2 — keycode strategy — **verified on both**

| Question | Result |
|---|---|
| Natural keycodes, ordinary keys | dev: `1` → keycode 10, `c` → 54. target: identical |
| Natural keycodes for JP keysyms | target (`layout: jp`): 半角/全角 → keycode 49, 英数 → 66, 変換 → 100, 無変換 → 102 — the same keycodes as the physical keys |
| Keysyms the active map cannot produce | Xvfb (US map): `Zenkaku_Hankaku` → spare keycode 8, `Eisu_toggle` → 97, all levels set to the same keysym |
| Server behaviour | the server stores 4 levels per spare; ownership is identified by level 0 |
| Warm reuse | a repeated tap reuses the same keycode (no remap churn) |
| Immunity to modifier state | sticky Shift held + spare key → keysym unchanged (`state 0x101`) |
| Restore on exit | `xmodmap -pke` md5 identical before/after on both machines, including a SIGTERM exit (INT/TERM/HUP handlers installed) |
| Map reset under us (`XkbMapNotify` from our own remap) | spares re-adopted (without this they were forgotten and leaked — real bug, fixed) |
| Map reset by someone else (Plasma layout daemon) | spares that no longer hold our keysym are dropped; the pool is rebuilt |
| **Spare modifier keycodes** | **probe fails**: after remapping an unused keycode to `Shift_L`, `XGetModifierMapping` does not list it (XKB derives modifiers from the compat rules, not from a raw keymap change). The backend therefore uses the *real* modifier keycodes — the fallback the plan allowed. OSK modifiers are pressed and released inside one script, so a physically held modifier is only at risk in the rare case of pressing the same modifier physically while an OSK chord is injected. |

## S3 — focusless floating window — **verified on the target (KWin) and fluxbox**

| Check | Result (target, KWin/Plasma X11) |
|---|---|
| `WM_HINTS.input` | `False` |
| Always on top | `_NET_WM_STATE_ABOVE, _NET_WM_STATE_STAYS_ON_TOP` |
| Taskbar/pager | `SKIP_TASKBAR, SKIP_PAGER`; window type `UTILITY` |
| All desktops | `_NET_WM_DESKTOP = 0xFFFFFFFF` (see the fix below) |
| Focus stays put | `XGetInputFocus` unchanged after clicking OSK keys; typing landed in Kate/Konsole/xev |
| Window placement | bottom-centre of the available geometry, above the Plasma panel |

Three KWin-specific fixes came out of this spike:

1. **Properties must be written before the window is mapped.** KWin reads
   `_NET_WM_DESKTOP` while it starts managing the window; writes issued after
   `show()` (the original ordering) were normalized back to the current desktop.
2. **KWin re-normalizes while managing**, so the request is repeated ~150 ms
   after mapping (and as an EWMH client message, which is also what makes a
   runtime "on all desktops" toggle work).
3. **Qt sets `_KDE_NET_WM_WINDOW_TYPE_OVERRIDE`** for tool windows; KWin then
   skips desktop handling entirely, so the adapter rewrites
   `_NET_WM_WINDOW_TYPE` to plain `UTILITY` before the map.

Also verified under fluxbox (second WM smoke test): the same hints, plus
fluxbox deriving `_NET_WM_STATE_STICKY` from `_NET_WM_DESKTOP=0xFFFFFFFF`, and
`startSystemMove()` (`_NET_WM_MOVERESIZE`) dragging the window. Without a WM
(Xvfb) the manual drag fallback takes over.

## S4 — tray + single instance — **verified on the target**

- Tray: the icon appears in the Plasma panel (screenshot check: a small
  keyboard glyph), and the application log stays empty, i.e.
  `QSystemTrayIcon::isSystemTrayAvailable()` was true.
  Note: Qt 5's `QSystemTrayIcon` uses **XEmbed** on X11; Plasma bridges it to
  its SNI tray through `xembedsniproxy` (running on the target). The plan's
  "SNI on Plasma" is therefore reached indirectly, with the same result.
- Single instance: `tabletkeyboard --lang jp106 --mode full` on the target
  forwarded its arguments to the running instance, which switched layout and
  resized (exit code 0).

## S5 — touch plumbing — **pending on the target**

Widgets do not accept `QTouchEvent`, so Qt synthesizes ordinary mouse events
from single touches — what `KeyButton` consumes. Multi-touch chords are not part
of v1. To verify on the device: taps produce one press/release pair, and the X
input focus does not move (same check as S3).

## Extra checks

- Unit tests pass on the target (Qt 5.15.3 / glibc 2.33) and on the dev machine.
- Kate (Qt/KF5, `fcitx5-qt`): ASCII typing, Ctrl+A/Ctrl+C/Ctrl+V round trip
  (`xyz` → `1xyz` in the saved file) and Ctrl+S.
- Konsole: `echo hello` typed entirely from the OSK printed `hello`; Ctrl+U
  (readline) cleared the line.
- KWin global shortcut: Alt+Tab pressed on the OSK switched the active window
  (`_NET_ACTIVE_WINDOW` changed from Konsole to Kate).
- Idle CPU: 0 s of CPU time over 5 s of idling; memory (Release, dev machine)
  RSS 86 MB / PSS 49 MB, mostly shared Qt/fontconfig pages.
- Rendering on the target panel: `us` and `jp106` full modes verified by
  screenshot (F-row, main block, nav cluster, numpad, JP labels 半/全, 英数,
  変換, 無変換, かな, ¥, ろ — no missing glyphs).
