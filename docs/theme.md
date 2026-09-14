# Theme format

Themes are JSON; the UI paints every key itself, so a theme is pure data.
Files live in `data/themes/` (bundled), `/usr/share/tabletkeyboard/themes/`, or
`~/.local/share/tabletkeyboard/themes/`. The `id` must equal the file name.

```json
{
  "id": "win10",
  "name": "Windows 10",
  "colors": {
    "key_top": "#fdfdfd",
    "key_bottom": "#f0f0f0",
    "key_border": "#c8c8c8",
    "key_text": "#1a1a1a",
    "key_pressed_top": "#dcdcdc",
    "key_pressed_bottom": "#c8c8c8",
    "key_hover": "#ecf6ff",
    "mod_active": "#cce4f7",
    "accent": "#0078d7",
    "bar_bg": "#f4f4f4",
    "window_bg": "#e8e8e8",
    "window_opacity": 1.0,
    "led_on": "#4caf50",
    "led_off": "#8a8a8a"
  },
  "metrics": {
    "radius": 3,
    "border": 1,
    "gap": 2,
    "padding": 4,
    "bar_height": 26,
    "font_family": "Noto Sans",
    "font_px": 15,
    "label_px": 11,
    "key_unit": 72
  }
}
```

## Colors (`#rrggbb`, optional `#rrggbbaa`)

| Name | Default | Used for |
|---|---|---|
| `key_top`, `key_bottom` | `#fdfdfd`, `#f0f0f0` | key gradient (top → bottom) |
| `key_border` | `#c8c8c8` | 1 px key outline |
| `key_text` | `#1a1a1a` | key label |
| `key_pressed_top`, `key_pressed_bottom` | `#dcdcdc`, `#c8c8c8` | key while pressed |
| `key_hover` | `#ecf6ff` | key under the pointer |
| `mod_active` | `#cce4f7` | sticky/locked modifier, lit Caps/Num indicator |
| `accent` | `#0078d7` | locked-modifier border, batch title-bar hover, warning glyph |
| `bar_bg` | `#f4f4f4` | title bar background |
| `window_bg` | `#e8e8e8` | window background behind the keys |
| `window_opacity` | `1.0` | whole-window opacity (0–1) |
| `led_on`, `led_off` | `#4caf50`, `#8a8a8a` | title-bar Num/Caps/Scroll indicators: lit and dark pill |

## Metrics

| Name | Default | Unit | Notes |
|---|---|---|---|
| `radius` | 3 | px | key corner radius |
| `border` | 1 | px | outline width |
| `gap` | 2 | px | space between keys/rows/blocks |
| `padding` | 4 | px | window padding |
| `bar_height` | 26 | px | title bar height |
| `font_family` | `Noto Sans` | – | labels |
| `font_px` | 15 | px | main label size |
| `label_px` | 11 | px | shifted hint / long labels |
| `key_unit` | 72 | px | 1 key unit (the shipped `minimal` theme uses 68) |

All pixel metrics are multiplied by the theme scale implied by the effective
key size, and key text shrinks automatically to fit narrow keys.

The effective key unit is `general/keyUnit` from the config file (default 72),
or the theme's `key_unit` when the config value is 0; it is then multiplied by
the Scale setting and capped by the fit-to-screen width. Sizes are Qt *logical*
pixels, so a HiDPI session scales them by its factor (see the "Sizing for
touch" note in the README).

## Behaviour that is not themeable

- Shifted symbols appear as small hints in the key's top-left corner (Windows
  OSK style) when the layout defines `shifted`.
- Locked modifiers get an `accent` border; sticky ones use `mod_active`.
- Letter labels switch to upper case while Shift is active.

## Adding a theme

```sh
cp data/themes/minimal.json ~/.local/share/tabletkeyboard/themes/mytheme.json
# edit id -> "mytheme", restart or pick it in Tray -> Theme
```

Malformed files are reported on stderr and skipped; unknown fields are ignored,
so a theme stays loadable across versions.
