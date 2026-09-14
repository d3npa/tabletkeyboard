// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 d3npa <gh@w1t.ch>
#include "platform/x11/keycodes.h"

#include <QVector>

#include <X11/Xlib.h>
#include <X11/keysym.h>

namespace osk {
namespace x11 {

KeycodeAllocator::KeycodeAllocator(Display *dpy) : dpy_(dpy)
{
    refresh();
}

void KeycodeAllocator::refresh()
{
    // Our own XChangeKeyboardMapping calls arrive back here as XkbMapNotify,
    // so remember the spares and re-adopt the ones that survived. Only a
    // foreign reset (e.g. Plasma's layout daemon) really clears them.
    const QHash<quint32, KeyCode> previousSpares = sparesFor_;

    naturalCache_.clear();
    sparesFor_.clear();
    spares_.clear();
    modifierKeycodes_.clear();
    freeKeycodes_.clear();
    map_.clear();

    if (!dpy_)
        return;

    int minKeycode = 0;
    int maxKeycode = 0;
    XDisplayKeycodes(dpy_, &minKeycode, &maxKeycode);
    if (minKeycode < 8)
        minKeycode = 8;
    if (maxKeycode < minKeycode)
        return;

    int perKeycode = 0;
    KeySym *symbols = XGetKeyboardMapping(dpy_, minKeycode, maxKeycode - minKeycode + 1, &perKeycode);
    if (!symbols || perKeycode <= 0) {
        if (symbols)
            XFree(symbols);
        return;
    }

    minKeycode_ = KeyCode(minKeycode);
    maxKeycode_ = KeyCode(maxKeycode);
    keysymsPerKeycode_ = perKeycode;
    map_.resize((maxKeycode - minKeycode + 1) * perKeycode);
    memcpy(map_.data(), symbols, size_t(map_.size()) * sizeof(KeySym));
    XFree(symbols);

    for (auto it = previousSpares.constBegin(); it != previousSpares.constEnd(); ++it) {
        const KeyCode keycode = it.value();
        if (keycode < minKeycode_ || keycode > maxKeycode_)
            continue;
        if (keycodeHasKeysymLive(keycode, KeySym(it.key()))) {
            sparesFor_.insert(it.key(), keycode);
            spares_.insert(keycode);
        }
    }

    for (int kc = minKeycode_; kc <= maxKeycode_; ++kc) {
        const KeyCode keycode = KeyCode(kc);
        if (keycodeIsFree(keycode))
            freeKeycodes_.append(keycode);
    }

    hasAltGr_ = XKeysymToKeycode(dpy_, XK_ISO_Level3_Shift) != 0;
}

KeySym KeycodeAllocator::keysymAt(KeyCode keycode, int level) const
{
    if (map_.isEmpty() || keycode < minKeycode_ || keycode > maxKeycode_)
        return NoSymbol;
    if (level < 0 || level >= keysymsPerKeycode_)
        return NoSymbol;
    return map_.at((keycode - minKeycode_) * keysymsPerKeycode_ + level);
}

bool KeycodeAllocator::keycodeIsFree(KeyCode keycode) const
{
    if (spares_.contains(keycode))
        return false;
    for (int level = 0; level < keysymsPerKeycode_; ++level) {
        if (keysymAt(keycode, level) != NoSymbol)
            return false;
    }
    return true;
}

bool KeycodeAllocator::keycodeHasKeysymLive(KeyCode keycode, KeySym keysym) const
{
    if (!dpy_)
        return false;
    int perKeycode = 0;
    KeySym *symbols = XGetKeyboardMapping(dpy_, keycode, 1, &perKeycode);
    if (!symbols || perKeycode <= 0) {
        if (symbols)
            XFree(symbols);
        return false;
    }
    // Level 0 identifies our remap: the server stores spare keycodes with all
    // its levels set to the same keysym, so any surviving level matches.
    const bool ours = symbols[0] == keysym;
    XFree(symbols);
    return ours;
}

void KeycodeAllocator::writeKeycode(KeyCode keycode, KeySym keysym)
{
    if (!dpy_ || keysymsPerKeycode_ <= 0)
        return;
    QVector<KeySym> symbols(keysymsPerKeycode_, keysym);
    XChangeKeyboardMapping(dpy_, keycode, keysymsPerKeycode_, symbols.data(), 1);
}

bool KeycodeAllocator::findNatural(quint32 keysym, KeyLocation *out)
{
    if (keysym == 0)
        return false;
    auto cached = naturalCache_.constFind(keysym);
    if (cached != naturalCache_.constEnd()) {
        *out = cached.value();
        return true;
    }
    if (map_.isEmpty() || keysymsPerKeycode_ <= 0)
        return false;

    const int levels = qMin(keysymsPerKeycode_, 4);
    for (int kc = minKeycode_; kc <= maxKeycode_; ++kc) {
        const KeyCode keycode = KeyCode(kc);
        if (spares_.contains(keycode))
            continue;
        for (int level = 0; level < levels; ++level) {
            if (keysymAt(keycode, level) != KeySym(keysym))
                continue;
            if (level >= 2 && !hasAltGr_)
                continue; // level 2/3 unreachable without AltGr
            const KeyLocation location{ keycode, level };
            naturalCache_.insert(keysym, location);
            *out = location;
            return true;
        }
    }
    return false;
}

KeyCode KeycodeAllocator::ensureSpare(quint32 keysym)
{
    if (!dpy_ || keysym == 0)
        return 0;
    const KeyCode existing = sparesFor_.value(keysym, 0);
    if (existing)
        return existing;

    while (!freeKeycodes_.isEmpty()) {
        const KeyCode keycode = freeKeycodes_.takeFirst();
        if (spares_.contains(keycode))
            continue;
        writeKeycode(keycode, KeySym(keysym));
        spares_.insert(keycode);
        sparesFor_.insert(keysym, keycode);
        return keycode;
    }
    return 0;
}

KeyCode KeycodeAllocator::modifierKeycode(quint32 modKeysym)
{
    const KeyCode cached = modifierKeycodes_.value(modKeysym, 0);
    if (cached)
        return cached;

    const KeyCode keycode = dpy_ ? XKeysymToKeycode(dpy_, KeySym(modKeysym)) : 0;
    modifierKeycodes_.insert(modKeysym, keycode);
    return keycode;
}

void KeycodeAllocator::restoreSpares()
{
    if (!dpy_ || keysymsPerKeycode_ <= 0) {
        sparesFor_.clear();
        spares_.clear();
        return;
    }
    for (auto it = sparesFor_.begin(); it != sparesFor_.end(); ++it) {
        const KeyCode keycode = it.value();
        // Only touch keycodes that still hold our keysym: if the mapping was
        // reset by someone else, it is no longer ours to clear.
        if (!keycodeHasKeysymLive(keycode, KeySym(it.key())))
            continue;
        writeKeycode(keycode, NoSymbol);
    }
    XSync(dpy_, False);
    sparesFor_.clear();
    spares_.clear();
    modifierKeycodes_.clear();
}

} // namespace x11
} // namespace osk
