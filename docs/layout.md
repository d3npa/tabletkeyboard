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

Blocks and rows may carry an `id`. The application hides the ids `frow` (the
F1–F12 row) and `numpad` unless the corresponding block toggle is on; hiding
`frow` also hides the `frowgap` rows (see below). Other ids are authoring aids.
A row is either a plain array of keys or an object with an `id` and a `keys`
array:

```json
{
  "name": "main",
  "blocks": [
    {
      "id": "main",
      "rows": [
        { "id": "frow", "keys": [ {"type":"key","label":"Esc","sym":"Escape"} ] },
        [ {"type":"key","label":"1","sym":"1","shifted":"exclam","fn":"F1"} ]
      ]
    },
    {
      "id": "nav",
      "rows": [
        { "id": "frowgap", "keys": [ {"type":"spacer","width":1}, {"type":"spacer","width":1} ] },
        [ {"type":"key","label":"PrtSc","sym":"Print"} ]
      ]
    }
  ]
}
```

A block can also be pushed down with `topGap` (in key units), but the shipped
layouts use a leading `frowgap` spacer row instead: it occupies exactly one row,
so the cluster stays level with the main block's number row whether or not the
F-row is shown (the gutter row is hidden together with `frow`).

## Key fields

| Field | Type | Default | Notes |
|---|---|---|---|
| `type` | string | `"key"` | `key`, `mod`, `action`, `spacer` |
| `label` | string | keysym's character or name | what is painted on the key |
| `width` | number | `1` | key units (1 unit = the base key) |
| `sym` | string | – | X keysym name, required for `type: key`; resolved with the keysym table and lint-checked |
| `shifted` | string | – | keysym sent while Shift is sticky/locked (drawn as a small hint on the key) |
| `fn` | string | – | keysym sent while Fn is armed (drawn instead of `label` while Fn is on) |
| `fnLabel` | string | keysym's character/name | label used for the `fn` keysym |
| `mod` | string | – | for `type: mod`: `shift`, `ctrl`, `alt`, `super`, `altgr`, `fn` |
| `action` | string | – | for `type: action`: `hide`, `toggle_mode`, `toggle_lang`, `layer` |
| `layer` | string | – | target layer name when `action` is `layer` |
| `kana` | string | – | printed kana legend, drawn small in the key's bottom-right corner; display only, never injected |
| `indicator` | string | – | `caps`, `num` or `scroll`: the key lights up with the X server's lock state |
| `repeat` | bool | `true` | hold-to-repeat (auto-disabled for indicator keys) |
| `height` | number | `1` | rows the key spans (stepped keycaps) |
| `topWidth` | number | `width` | width of the key's first row unit when it is wider than `width` |

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

`fn` is the one modifier that is not pressed through X — it has no keysym of
its own. When it is armed, a key with an `fn` keysym sends that keysym instead
(all other keys behave as usual), and Fn alone injects nothing. The shipped
layouts put F1–F12 and Esc on the number row's `fn` so the F-row block can stay
hidden.

## Stepped keys (JIS Return)

A key with `height > 1` spans several rows; `topWidth` widens its first row
unit, which is what makes the JIS Return L-shaped:

```json
{ "type": "key", "label": "⏎", "sym": "Return", "width": 1.25, "topWidth": 1.5, "height": 2, "repeat": false }
```

The key is 1.5 units wide in the Tab row and 1.25 units wide in the home row,
right-aligned, so the step (notch) sits at its lower left, next to `[` and `]`.
The shape is inset by one `gap` on its left and top edges (the right and bottom
edges stay on the cell), which gives the wide part the same breathing room
against `[`, `]` and the row above as flat keys have.
The area of that notch belongs to the key below-left of it (`]`), as on a
physical JIS board: a press there types `]`, and if nothing covers it the press
is passed to the window (drag) instead of the Return key.

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

Checks JSON structure, unknown/invalid keysyms (including `fn`), unknown
modifiers and action targets, duplicate layer names, empty rows, non-positive
widths, `height < 1` and `topWidth <= 0`. It also warns when a stepped key
(`topWidth > width`) is not the last key of its row, because its step would
overlap the next key. The unit tests run the same lint over the bundled
layouts.

## Authoring notes

- Rows are centred inside their block, so rows of different total width stay
  symmetric; make the main rows equal (15 units for a 104-key layout) and use
  `spacer` for gaps.
- A stepped key must be the last key of its row. The row's width counts its
  `topWidth`, so the rows it spans keep the block's total width.
- The window is scaled to fit the screen width: a very wide layout gets smaller
  keys, never clipping horizontally.
- Group related keys into blocks; the nav/numpad clusters in `us.json` are a
  good template. Give toggleable groups the ids `frow` and `numpad` so the
  Blocks menu can hide them.
