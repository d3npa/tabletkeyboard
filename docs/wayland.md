# Wayland outlook

Research notes (2026-09-14) that shape the platform abstraction, plus the
interface conclusion for a future backend. No v1 code depends on any of this.

## Findings

- **KWin does not implement `zwp_virtual_keyboard_v1`** (raw key injection).
  There is a KDE bug requesting it; the supported architecture is
  **input-method-v1**, the same path `plasma-keyboard` (Qt Virtual Keyboard) and
  Maliit use. Input-method protocols commit *text*, which covers text entry but
  not arbitrary shortcuts (Alt+Tab, Ctrl+C in an app that does not accept IME
  commits).
- **Consequence**: a Wayland backend is not "the same injection with another
  API". The realistic split is `input-method-v1` for text entry into IME-aware
  applications — text goes through the compositor, *not* through fcitx5 — plus
  either `uinput` (ydotool-style, needs permissions/udev) or compositor-specific
  hooks for raw keys and shortcuts.
- **XWayland caveat**: in a Wayland session XTEST can only drive XWayland
  clients, so it is not a general fallback.
- Qt Widgets itself runs fine on Wayland, so the UI layer (`osk-ui`) survives
  the port unchanged; only `osk-platform` gains a second implementation.

## What this means for the current design

`osk-core` (layout/mode/language model, sticky state machine, themes, lint)
already knows nothing about X11 or about how a key becomes an event. The
platform interface should therefore be split by *capability* rather than assume
both always exist:

```
InputBackend
├── key events      (X11: XTEST · Wayland: uinput / compositor hooks)
└── text commit     (Wayland: input-method-v1 · X11: not needed)
```

The current X11 backend implements the key-event half; `InputBackend` is
deliberately small (`available()`, `execute(KeyScript)`, `syncKeymap()`,
`shutdown()`) so a Wayland implementation can expose capability flags instead
of pretending to support both. The language switch and layout data already treat
"text" as sequences of keysyms, which maps cleanly onto input-method commits.

`WindowAdapter` (EWMH/ICCCM behaviour today) has the same shape: a Wayland
implementation would use layer-shell + keyboard-interactivity hints instead of
`_NET_WM_STATE`/`_NET_WM_DESKTOP`, and `Qt::WindowDoesNotAcceptFocus` is
already the portable way to stay focusless.

## Not planned for v1

No Wayland backend, no `uinput`, no compositor-specific hacks: v1 targets
KDE5/X11 exactly, with the seams above in place so the port is additive.
