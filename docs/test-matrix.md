# Test matrix

Run on the target device (Let's Note CF-QV, Slackware 15.0, KDE Plasma 5/X11,
fcitx5 with `fcitx5-qt` + `fcitx5-mozc`), with the same fcitx5 configuration as
physical typing. Rerun after any change that touches injection.

Legend: ✓ = executed, ✗ = failed, `–` = not applicable, **pending** = not yet
executed.

| Target | ASCII typing | JP compose → 変換 → Enter | 半角/全角 toggle | Ctrl+C/V, Alt+Tab | Focus preserved |
|---|---|---|---|---|---|
| Konsole (Qt, fcitx5-qt) | ✓ `echo hello` → `hello` | – | – | ✓ Alt+Tab switched the active window (KWin); ✓ Ctrl+U (readline) | ✓ |
| Kate (Qt/KF5) | ✓ `hello` saved | ✓ `感じ`, `私は学生です` | ✓ `fcitx5-remote` 1 → 2 | ✓ Ctrl+A/Ctrl+C/Ctrl+V round trip (`xyz` → `1xyz`); Ctrl+S | ✓ |
| Firefox (GTK3, fcitx5-gtk) | pending | pending | pending | pending | pending |
| Thunderbird | pending | pending | pending | pending | pending |
| `xterm` (XIM via `XMODIFIERS`) | pending (not installed on the target) | | | | |

Notes from the target run:

- All of the above was produced by clicking the on-screen keyboard only; the
  receiver applications were focused beforehand and never lost focus.
- Kate's JP row is also the P4 acceptance test: a sentence was composed and
  converted to kanji purely from the OSK, then written to disk with the OSK's
  Ctrl+S.
- 変換/無変換 behave exactly like the physical keys because the injected events
  carry the same keycodes (100/102 on the `jp` map) and keysyms; on this machine
  無変換 is bound to `[Hotkey/DeactivateKeys]` in `~/.config/fcitx5/config`, so
  it deactivates the IME — the OSK does not change that.
- Firefox/Thunderbird/xterm still need a pass; the mechanism is identical
  (XTEST into the focused window), but these use different IME front-ends
  (GTK module and XIM), so they are worth running.

## Development-machine harness (Xvfb, and fluxbox as a second WM)

Receiver: `xev`; driver: XTEST clicks on the OSK.

| Scenario | Expectation | Result |
|---|---|---|
| Tap `1` in `us` full | `keycode 10 (keysym 0x31, 1)`, `synthetic NO` | pass |
| Sticky Ctrl then `c` | `Control_L` press, `c` with `XLookupString 0x03`, both released | pass |
| Sticky Shift then a spare-key keysym | keysym unchanged with Shift active (`state 0x101`) | pass |
| Tap 変換/半角全角 with a US map | natural keycode / spare keycode, all levels equal | pass |
| Exit (SIGTERM) after using spares | `xmodmap -pke` identical to before startup | pass |
| Click OSK keys while `xev` is focused | focus still `xev`, with and without a WM | pass |
| EWMH hints under fluxbox | `input=False`, `ABOVE`, `SKIP_TASKBAR/PAGER`, `STICKY` | pass |
| WM-assisted drag (`startSystemMove`) | window follows the drag under fluxbox | pass |
| Second instance forwards `--mode/--lang` | running keyboard switches and resizes | pass |
| Hide/show via CLI | window unmaps and remaps; position clamped on-screen | pass |
| Idle CPU | ~0 % (0 s of CPU over 5 s) | pass |

## v2 pass (dark default, blocks, Fn layer, JIS Return, key size)

Same harness (Xvfb 1600×1000 + `xev`, and the target device with KWin, Plasma
scaling 2×). Driver: XTEST clicks on the OSK at coordinates derived from the
layout geometry.

| Scenario | Expectation | Result |
|---|---|---|
| Fresh config, `--show` | dark theme (`win10-dark`), no F-row, no numpad; window 1336×404 on 1600×1000, 5 rows, 72 px keys | pass |
| Fn (us) then `1` | exactly one `Tap(F1)`; no `1`; Fn one-shot consumed (unit test + xev: 2 `0xffbe` lines) | pass |
| Shift + Fn then `1` | `[Shift↓, F1, Shift↑]`, both one-shots consumed | pass (unit test) |
| Fn alone | no injected events | pass (unit test) |
| jp106 Return, lower part | `keysym 0xff0d, Return` (dev screen and device) | pass |
| jp106 Return, upper part | `keysym 0xff0d, Return` | pass |
| jp106 Return, notch (0.25 u left of the body) | the `]` key answers (`0x5d, bracketright`), never Return; where nothing covers the notch the press falls through to the window | pass |
| Title-bar Dark/Light button | flips live on click: screenshot mean 0.197 → 0.946 → 0.197 | pass |
| `--light` / `--dark` | light mean 0.94, dark mean 0.20 | pass |
| `--blocks frow,numpad` | window grows (5 → 6 rows, numpad block appears), screenshot shows Esc/F1–F12 and the numpad | pass |
| `general/keyUnit=100` | window 1592×481 (vs 1336×404) — bigger keys; the value survives the app's config rewrite | pass |
| `general/keyUnit=200` | clamped by the fit-to-screen unit (~86 px): same window as 100, keys are not 200 px | pass |
| Device (KWin, 2× scaling, jp106) | 2676×808 physical window, dark (mean 0.199), 5 rows, no numpad, stepped Return visible | pass |
| Device Fn + `1` | `keysym 0xffbe, F1`, no plain `1` | pass |
| Device 半/全 in Kate | `fcitx5-remote` 1 → 2 → 1 | pass |
| Unit tests (`tst_layout`, `tst_geometry`, `tst_keystate`, `tst_theme`) | pass on Qt 5.15.19 (dev) and Qt 5.15.3 (target) | pass |

Notes:

- The notch of the JIS Return is physically the domain of the `]` key (the
  Return's upper part is 0.25 u wider than its lower part), so pressing it types
  `]` — the same as on hardware. The key widget still ignores presses in the
  notch so the window can be dragged there when no key covers it.
- Key sizes are Qt logical pixels: the target session runs at a 2× scale
  factor, so the default 72 px is 144 physical px ≈ 38 mm there; `keyUnit=36`
  gives ≈19 mm. See the README's "Sizing for touch".

## Performance targets

| Target | Requirement | Measured |
|---|---|---|
| tap → character appears | < 50 ms | instant: the injected event carries the same timestamp as the click; text appears immediately on the target |
| idle CPU | ≈ 0 % | 0 s of CPU time accrued over 5 s of idling (no polling; X events arrive via `QSocketNotifier`) |
| memory | < 50 MB | Release build, running: RSS 86 MB / **PSS 49 MB** (RSS counts the shared Qt, DBus and fontconfig pages in full; PSS attributes them proportionally). Debug build: PSS 50 MB. |

Qt5 Widgets itself accounts for most of the footprint; `osk-core`, all layout
and theme data and the state machine are a few hundred kB.
