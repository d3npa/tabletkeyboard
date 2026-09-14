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

## Performance targets

| Target | Requirement | Measured |
|---|---|---|
| tap → character appears | < 50 ms | instant: the injected event carries the same timestamp as the click; text appears immediately on the target |
| idle CPU | ≈ 0 % | 0 s of CPU time accrued over 5 s of idling (no polling; X events arrive via `QSocketNotifier`) |
| memory | < 50 MB | Release build, running: RSS 86 MB / **PSS 49 MB** (RSS counts the shared Qt, DBus and fontconfig pages in full; PSS attributes them proportionally). Debug build: PSS 50 MB. |

Qt5 Widgets itself accounts for most of the footprint; `osk-core`, all layout
and theme data and the state machine are a few hundred kB.
