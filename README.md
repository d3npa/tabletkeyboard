# tabletkeyboard

<img src="docs/img/tabletkeyboard-1.jpg" width="49%" alt="tabletkeyboard over kde's about this system window on a let's note, jp106 layout"> <img src="docs/img/tabletkeyboard-2.jpg" width="49%" alt="tabletkeyboard over kde's about this system window, jp106 layout">

an on-screen keyboard for x11. keys are injected with xtest, so fcitx5, applications and shortcuts see ordinary key presses - no root, no `uinput`, no configuration changes. the window is frameless, always on top and never takes focus, so whatever you were typing in keeps it.

i use it on a let's note cf-qv (slackware 15.0, kde plasma 5, x11), but it is only an x11 client.

## build

needs qt5 (core, gui, widgets, network), libxtst, cmake and a c++17 compiler.

	cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
	cmake --build build -j$(nproc)
	sudo cmake --install build

there is a thin make wrapper if you prefer: `make`, `make test`, `make install PREFIX=/usr`.

## run

	tabletkeyboard          # tray only
	tabletkeyboard --show
	tabletkeyboard --toggle

a second invocation forwards its arguments to the running instance. `--help` lists the rest.

## customise

layouts and themes are plain json - drop your own in and restart:

	~/.local/share/tabletkeyboard/layouts/*.json
	~/.local/share/tabletkeyboard/themes/*.json

the bundled `us` and `jp106` layouts and the four themes live in `data/`. the fields are described in [docs/layout.md](docs/layout.md) and [docs/theme.md](docs/theme.md), and a layout can be checked before you use it:

	tabletkeyboard --check-layout mylayout.json

everything else - theme, scale, key size, f-row, numpad, sticky modifiers, start at login - is in the tray menu, the settings dialog, or `~/.config/tabletkeyboard/tabletkeyboard.conf`.

## notes

- wayland is not supported: xtest only reaches xwayland windows, see [docs/wayland.md](docs/wayland.md).
- only really tested on my let's note so far; [docs/test-matrix.md](docs/test-matrix.md) has what was and was not covered.
- gpl-2.0-or-later, see [LICENSE](LICENSE).
