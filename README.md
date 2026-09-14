# tabletkeyboard

A zero-privilege on-screen keyboard for X11. Keys are injected through
**XTEST**, so they enter the X server's normal input path: fcitx5, every
application and every shortcut behave exactly as they do for the physical
keyboard. No root, no `uinput`, no window-manager or fcitx5 configuration.

Target: KDE Plasma 5 on X11 (Slackware 15.0); the keyboard is WM-agnostic and
only uses ICCCM/EWMH + XTEST.

## Features

- **Data-driven layouts**: `us` and `jp106` ship as JSON, each with a `full`
  mode (desktop keyboard, F-row, nav cluster, numpad) and a `simple` mode
  (thumb typing with a symbols layer).
- **Sticky modifiers**: Shift/Ctrl/Alt/Super/AltGr — tap once for one-shot, tap
  again or long-press to lock; state is visible on the key. CapsLock/NumLock
  are read from the X server (`XkbStateNotify`) and shown truthfully.
- **Floating focusless window**: frameless, always-on-top, `WM_HINTS.input =
  False`, skip-taskbar, sticky across desktops. Drag anywhere on the bar or the
  gaps; the position is remembered per screen. The window never takes focus, so
  injected keys always land in the application you were typing in.
- **Themes**: JSON (`win10` default, `win10-dark`, `minimal`); custom-painted
  keys, no stylesheet magic.
- **Tray icon** (`QSystemTrayIcon`; on X11 this is an XEmbed item, which Plasma
  bridges to its SNI tray through `xembedsniproxy`): show/hide, mode, language,
  theme, scale, start-at-login, quit.
- **Single instance**: a second invocation forwards its command line to the
  running instance.
- **Key hold repeats** (Backspace, arrows, …), hold-to-lock modifiers, and an
  optional sticky auto-clear timeout.
- **Core stays portable**: `osk-core` is QtCore-only and X11-free; the platform
  layer is one `InputBackend` plus one `WindowAdapter`, so a Wayland backend can
  be added without touching the core (see `docs/wayland.md`).

## Requirements

- Qt 5.15 (Core, Gui, Widgets, Network; Test for the unit tests)
- X11 with the XTEST extension (`libXtst`)
- CMake ≥ 3.16, a C++17 compiler

## Build and install

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"
ctest --test-dir build            # unit tests
sudo cmake --install build        # /usr/local (use -DCMAKE_INSTALL_PREFIX=/usr)
```

Slackware package: `cd packaging/slackware && ./tabletkeyboard.SlackBuild`.

## Run

```sh
tabletkeyboard                  # hidden, tray only (no tray: window at startup)
tabletkeyboard --show
tabletkeyboard --toggle
tabletkeyboard --mode simple --lang jp106
tabletkeyboard --theme win10-dark --scale 1.25
tabletkeyboard --check-layout data/layouts/jp106.json
```

| Option | Effect |
|---|---|
| `--show` / `--hide` / `--toggle` | window visibility |
| `--mode full\|simple` | keyboard mode |
| `--lang ID` | layout set (`us`, `jp106`, …) |
| `--theme ID` | theme (`win10`, `win10-dark`, `minimal`, …) |
| `--scale FACTOR` | keyboard scale, 0.5 – 3.0 |
| `--check-layout FILE` | lint a layout file against the current display, then exit |
| `--version`, `--help` | |

All of these work on a running instance too: the second process forwards its
arguments and exits.

## Configuration

`~/.config/tabletkeyboard/tabletkeyboard.conf` (QSettings format):

| Key | Default | Meaning |
|---|---|---|
| `general/scale` | `1.0` | keyboard scale |
| `general/stickyTimeoutMs` | `0` | auto-clear a one-shot modifier after N ms (0 = never) |
| `general/zenkakuOnLangSwitch` | `false` | also send 半角/全角 when switching language (opt-in Windows-like coupling; the OSK never touches fcitx5 otherwise) |
| `general/startAtLogin` | `false` | mirrors the autostart file |
| `general/onAllDesktops` | `true` | `_NET_WM_DESKTOP = 0xFFFFFFFF` |
| `general/theme`, `general/layout`, `general/mode` | `win10`, `us`, `full` | last used |
| `position/<screen>` | – | window position per screen name |

## Layouts and themes

Drop-in files (the `id` must match the file name):

```
~/.local/share/tabletkeyboard/layouts/*.json
~/.local/share/tabletkeyboard/themes/*.json
```

Bundled copies live in `data/` and are also compiled into the binary, so the
application works uninstalled. Search order: user dir → `/usr/share/tabletkeyboard`
→ built-in.

- Layout schema and authoring guide: [docs/layout.md](docs/layout.md)
- Theme schema: [docs/theme.md](docs/theme.md)

You can add a language with no code changes: write `layouts/<id>.json`, run
`tabletkeyboard --check-layout` on it, and it appears in the language menu.

## How typing works

1. A key tap reaches `KeyStateMachine` (sticky state, layers, modes, language).
2. It emits a `KeyScript` of keysyms: `[Shift↓, A, Shift↑]` for a sticky shift,
   `[Ctrl↓, c, Ctrl↑]` for Ctrl+C.
3. The X11 backend resolves each keysym against the active keyboard map and
   injects it with `XTestFakeKeyEvent` — the same events a physical keyboard
   produces, so fcitx5 (XIM or the Qt/GTK modules) sees identical input.

Keysyms the active map cannot produce (for example `Zenkaku_Hankaku` on a US
map) are sent through a **spare keycode**: an unused keycode is remapped to the
keysym with every level set the same, so no modifier state can change the
result. Spares are reused, re-created after an external map reset
(`XkbMapNotify`), and restored on exit — including on SIGTERM/SIGINT/SIGHUP.

Modifiers are pressed through their real keycodes (see `docs/spikes.md` for the
spare-modifier probe result).

## Troubleshooting

- **A ⚠ appears in the title bar** — XTEST is missing on this display
  (`xdpyinfo | grep -i xtest`). The keyboard cannot inject anything.
- **No tray icon under Plasma** — Plasma hosts XEmbed tray icons through
  `xembedsniproxy`: check that it is running (`pgrep -a xembedsniproxy`) and that
  the tray widget is present. Without any tray the keyboard shows itself at
  startup so it stays reachable.
- **No Japanese glyphs** — `fc-list :lang=ja`; pick a font with CJK coverage in
  the theme's `font_family`.
- **Tray/Plasma resets the keyboard map** (layout switch) — harmless: the
  backend re-syncs and re-creates spare keycodes on demand.
- **Wayland session** — XTEST only reaches XWayland clients; see
  `docs/wayland.md`.

## Status

Verified end-to-end on the target device (Let's Note CF-QV, Slackware 15.0,
KDE Plasma 5/X11, fcitx5-mozc): XTEST injection, focusless window and EWMH
hints under KWin, tray icon, Alt+Tab through KWin, ASCII and Ctrl+C/V in Kate
and Konsole, and a full Japanese sentence composed and converted purely from the
OSK. Details and remaining items: `docs/spikes.md` and `docs/test-matrix.md`.

## License

GPL-2.0-or-later, see [LICENSE](LICENSE).
