// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 d3npa <gh@w1t.ch>
#include "platform/x11/xconnection.h"

#include <QSocketNotifier>

#include <X11/XKBlib.h>
#include <X11/extensions/XTest.h>
#include <X11/keysym.h>

namespace osk {

namespace {

// Xlib's default handler terminates the process on some errors, which must not
// happen for a failed request on the private connection. Xlib exposes no
// request-name table, so log the request codes and the error text instead.
int xErrorHandler(Display *display, XErrorEvent *event)
{
    char text[128] = {};
    XGetErrorText(display, event->error_code, text, sizeof(text));
    qWarning("tabletkeyboard: X error: %s (request %d.%d, resource 0x%lx)", text, int(event->request_code),
             int(event->minor_code), event->resourceid);
    return 0; // keep going
}

} // namespace

X11Connection::X11Connection(QObject *parent) : QObject(parent) {}

X11Connection::~X11Connection()
{
    if (display_)
        XCloseDisplay(display_);
}

bool X11Connection::open(QString *error)
{
    display_ = XOpenDisplay(nullptr);
    if (!display_) {
        if (error)
            *error = QStringLiteral("cannot open the X display (is DISPLAY set?)");
        return false;
    }

    // Process-global by Xlib design; Qt's XCB connection reports its errors
    // through XCB, so this only covers the private connection.
    XSetErrorHandler(xErrorHandler);

    int eventBase = 0;
    int errorBase = 0;
    int major = 0;
    int minor = 0;
    xtestOk_ = XTestQueryExtension(display_, &eventBase, &errorBase, &major, &minor) == True;

    int opcode = 0;
    xkbOk_ = XkbQueryExtension(display_, &opcode, &xkbEventBase_, &errorBase, &major, &minor) == True;
    if (xkbOk_) {
        XkbSelectEvents(display_, XkbUseCoreKbd, XkbStateNotifyMask | XkbMapNotifyMask,
                        XkbStateNotifyMask | XkbMapNotifyMask);

        readLockMasks();

        XkbStateRec state{};
        if (XkbGetState(display_, XkbUseCoreKbd, &state) == Success)
            updateLockState(state.locked_mods);
    }

    notifier_ = new QSocketNotifier(ConnectionNumber(display_), QSocketNotifier::Read, this);
    connect(notifier_, &QSocketNotifier::activated, this, &X11Connection::processEvents);

    return true;
}

void X11Connection::processEvents()
{
    if (!display_)
        return;

    while (XPending(display_) > 0) {
        XEvent event;
        XNextEvent(display_, &event);
        if (!xkbOk_ || event.type != xkbEventBase_ + XkbEventCode)
            continue;

        XkbEvent *xkbEvent = reinterpret_cast<XkbEvent *>(&event);
        switch (xkbEvent->any.xkb_type) {
        case XkbStateNotify:
            updateLockState(xkbEvent->state.locked_mods);
            break;
        case XkbMapNotify:
            // `xmodmap -e 'add mod3 = Scroll_Lock'` and friends land here: the
            // lock masks belong to the map, so re-read them and the state.
            readLockMasks();
            {
                XkbStateRec state{};
                if (XkbGetState(display_, XkbUseCoreKbd, &state) == Success)
                    updateLockState(state.locked_mods);
            }
            emit keymapChanged();
            break;
        default:
            break;
        }
    }
}

void X11Connection::readLockMasks()
{
    numLockMask_ = 0;
    scrollLockMask_ = 0;

    XModifierKeymap *modifierMap = XGetModifierMapping(display_);
    const KeyCode numLock = XKeysymToKeycode(display_, XK_Num_Lock);
    const KeyCode scrollLock = XKeysymToKeycode(display_, XK_Scroll_Lock);
    const int perMod = modifierMap ? modifierMap->max_keypermod : 0;
    for (int mod = 0; modifierMap && mod < 8; ++mod) {
        for (int slot = 0; slot < perMod; ++slot) {
            const KeyCode code = modifierMap->modifiermap[mod * perMod + slot];
            if (!code)
                continue;
            if (numLock && code == numLock)
                numLockMask_ |= 1u << mod;
            if (scrollLock && code == scrollLock)
                scrollLockMask_ |= 1u << mod;
        }
    }
    if (modifierMap)
        XFreeModifiermap(modifierMap);

    // Num Lock has a conventional home even when nothing binds it; Scroll Lock
    // does not, so an unbound Scroll_Lock simply keeps its indicator dark.
    if (!numLockMask_)
        numLockMask_ = Mod2Mask;
}

void X11Connection::updateLockState(unsigned int lockedMods)
{
    const bool caps = (lockedMods & LockMask) != 0;
    const bool num = numLockMask_ != 0 && (lockedMods & numLockMask_) != 0;
    const bool scroll = scrollLockMask_ != 0 && (lockedMods & scrollLockMask_) != 0;

    if (caps != capsOn_) {
        capsOn_ = caps;
        emit capsLockChanged(caps);
    }
    if (num != numOn_) {
        numOn_ = num;
        emit numLockChanged(num);
    }
    if (scroll != scrollOn_) {
        scrollOn_ = scroll;
        emit scrollLockChanged(scroll);
    }
}

} // namespace osk
