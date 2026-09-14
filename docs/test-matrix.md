# Test matrix

Run on the target device (Let's Note CF-QV, Slackware 15.0, KDE Plasma 5/X11,
fcitx5 with `fcitx5-qt` + `fcitx5-mozc`), with the same fcitx5 configuration as
physical typing. Rerun after any change that touches injection.

**Status: not yet executed on the target.** The rows below are the procedure;
`docs/spikes.md` records what could be verified on the development machine
(Xvfb + fluxbox + `xev`), which covers injection mechanics but not IMEs.

| Target | ASCII typing | JP compose → 変換 → Enter | 半角/全角 toggle | Ctrl+C/V, Alt+Tab | Focus preserved |
|---|---|---|---|---|---|
| Konsole (Qt, fcitx5-qt) | pending | pending | pending | pending | pending |
| Kate (Qt/KF5) | pending | pending | pending | pending | pending |
| Firefox (GTK3, fcitx5-gtk) | pending | pending | pending | pending | pending |
| Thunderbird | pending | pending | pending | pending | pending |
| `xterm` (XIM via `XMODIFIERS`) | pending | pending | pending | pending | pending |

## Local harness results (development machine, Xvfb :99)

Receiver: `xev` (focused); driver: XTEST clicks on the OSK (throwaway helper).

| Scenario | Expectation | Result |
|---|---|---|
| Tap `1` in `us` full | `keycode 10 (keysym 0x31, 1)`, `synthetic NO` | pass |
| Sticky Ctrl then `c` | `Control_L` press, `c` with `XLookupString 0x03`, both released | pass |
| Sticky Shift then a spare-key keysym | keysym unchanged with Shift active (`state 0x101`) | pass |
| Tap 変換 (`Henkan`) with a US map | `keycode 100 (keysym 0xff23)`, natural keycode | pass |
| Tap 半/全 (`Zenkaku_Hankaku`) with a US map | spare keycode remapped, all levels equal | pass |
| Second tap of the same spare keysym | same keycode reused (no remap) | pass |
| Exit (SIGTERM) after using spares | `xmodmap -pke` identical to before startup | pass |
| Click OSK keys while `xev` is focused | focus still `xev`, no WM and under fluxbox | pass |
| EWMH hints under fluxbox | `input=False`, `ABOVE`, `SKIP_TASKBAR/PAGER`, `STICKY` | pass |
| WM-assisted drag (`startSystemMove`) | window follows the drag under fluxbox | pass |
| Second instance forwards `--mode/--lang` | running keyboard switches and resizes | pass |
| Hide/show via CLI | window unmaps and remaps; position clamped on-screen | pass |
| Idle CPU | ~1 % of one core | pass |

## Performance targets

| Target | Requirement | Measured |
|---|---|---|
| tap → character appears | < 50 ms | instant in `xev` (the injected event carries the same timestamp as the click) |
| idle CPU | ≈ 0 % | 0 s of CPU time accrued over 5 s of idling (no polling; X events arrive via `QSocketNotifier`) |
| memory | < 50 MB | Release build, running: RSS 86 MB / **PSS 49 MB** (RSS counts the shared Qt, DBus and fontconfig pages in full; PSS attributes them proportionally). Debug build: PSS 50 MB. |

Qt5 Widgets itself accounts for most of the footprint; `osk-core`, all layout
and theme data and the state machine are a few hundred kB.
