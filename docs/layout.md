# Layout format

A layout set describes one language's keys for both keyboard modes. Files live
in `data/layouts/<id>.json` (bundled), `/usr/share/tabletkeyboard/layouts/`, or
`~/.local/share/tabletkeyboard/layouts/`. The `id` field must equal the file
name.

## Structure

```json
{
  "id": "us",
  "name": "English (US)",
  "modes": {
    "full":   { "layers": [ /* … */ ] },
    "simple": { "layers": [ /* … */ ] }
  }
}
```

| Level | Meaning |
|---|---|
| layout | `id`, display `name`, `modes` |
| mode | one keyboard mode (`full`, `simple`, …); the title-bar button cycles them |
| layer | a named key grid inside a mode (`main`, `symbols`, …); the first layer is the default |
| block | a column of rows; blocks are laid out left to right (main keyboard, nav cluster, numpad) |
| row | a list of keys; rows are centred inside their block |
| key | see below |

```json
{
  "name": "main",
  "blocks": [
    {
      "rows": [
        [ {"type":"key","label":"1","sym":"1","shifted":"exclam"} ],
        [ {"type":"key","label":"⌫","sym":"BackSpace","width":2} ]
      ]
    },
    { "topGap": 1, "rows": [ /* nav cluster */ ] }
  ]
}
```

`topGap` (in key units) pushes a block down so side clusters line up with the
main block.

## Key fields

| Field | Type | Default | Notes |
|---|---|---|---|
| `type` | string | `"key"` | `key`, `mod`, `action`, `spacer` |
| `label` | string | keysym's character or name | what is painted on the key |
| `width` | number | `1` | key units (1 unit = the base key) |
| `sym` | string | – | X keysym name, required for `type: key`; resolved with the keysym table and lint-checked |
| `shifted` | string | – | keysym sent while Shift is sticky/locked (drawn as a small hint on the key) |
| `mod` | string | – | for `type: mod`: `shift`, `ctrl`, `alt`, `super`, `altgr` |
| `action` | string | – | for `type: action`: `hide`, `toggle_mode`, `toggle_lang`, `layer` |
| `layer` | string | – | target layer name when `action` is `layer` |
| `indicator` | string | – | `caps` or `num`: the key lights up with the X server's lock state |
| `repeat` | bool | `true` | hold-to-repeat (auto-disabled for indicator keys) |
| `longpress` | – | – | reserved; ignored by this version |

Keysyms are resolved against the X keysym table (`XStringToKeysym`), so any
name from `keysymdef.h` works: `Henkan`, `Muhenkan`, `Hiragana_Katakana`,
`Zenkaku_Hankaku`, `KP_Add`, `Prior` (PgUp), `Next` (PgDn), `ISO_Level3_Shift`,
`yen`, `bar`, …

**Note**: `Katakana_Hiragana` does not exist; the keysym is `Hiragana_Katakana`.
Japanese IME keys are plain keysyms — 半角/全角 `Zenkaku_Hankaku`, 変換 `Henkan`,
無変換 `Muhenkan`, かな `Hiragana_Katakana` — which fcitx5 already binds. No IME
integration code exists or is needed.

Layouts do not depend on the active keyboard map: a keysym the map cannot
produce is injected through a spare keycode (see README). `--check-layout`
verifies names and structure, not what the current map can produce.

## Modifier keys

`type: mod` keys are sticky:

| Gesture | Result |
|---|---|
| tap | one-shot — applies to the next key tap, then clears |
| tap again (or long-press) | locked — applies to every following tap until tapped again |
| long-press while locked | unlocks |

Shift selects `shifted` if the key defines one; otherwise the base keysym is
sent and the X server applies the shift level. CapsLock is never simulated
locally: the Caps key sends a normal `Caps_Lock` press and XKB decides letter
case, exactly like hardware.

## Actions

| `action` | Effect |
|---|---|
| `hide` | hide the window |
| `toggle_mode` | switch between the layout's modes (`full` ⇄ `simple`) |
| `toggle_lang` | switch to the next layout set (language) |
| `layer` | show the layer named in `layer` (e.g. `&123` ⇄ `ABC`) |

## Lint

```sh
tabletkeyboard --check-layout path/to/layout.json
```

Checks JSON structure, unknown/invalid keysyms, unknown modifiers and action
targets, duplicate layer names, empty rows and non-positive widths. The unit
tests run the same lint over the bundled layouts.

## Authoring notes

- Rows are centred inside their block, so rows of different total width stay
  symmetric; make the main rows equal (15 units for a 104-key layout) and use
  `spacer` for gaps.
- The window is scaled to fit the screen width: a very wide layout gets smaller
  keys, never clipping.
- Group related keys into blocks; the nav/numpad clusters in `us.json` are a
  good template.
