#include "platform/x11/xconnection.h"

#include <QSocketNotifier>

#include <X11/XKBlib.h>
#include <X11/extensions/XTest.h>
#include <X11/keysym.h>

namespace osk {

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

        XModifierKeymap *modifierMap = XGetModifierMapping(display_);
        const KeyCode numLock = XKeysymToKeycode(display_, XK_Num_Lock);
        if (modifierMap && numLock) {
            for (int mod = 0; mod < 8; ++mod) {
                for (int slot = 0; slot < modifierMap->max_keypermod; ++slot) {
                    if (modifierMap->modifiermap[mod * modifierMap->max_keypermod + slot] == numLock)
                        numLockMask_ = 1u << mod;
                }
            }
        }
        if (modifierMap)
            XFreeModifiermap(modifierMap);
        if (!numLockMask_)
            numLockMask_ = Mod2Mask; // XKB convention

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
            emit keymapChanged();
            break;
        default:
            break;
        }
    }
}

void X11Connection::updateLockState(unsigned int lockedMods)
{
    const bool caps = (lockedMods & LockMask) != 0;
    const bool num = numLockMask_ != 0 && (lockedMods & numLockMask_) != 0;

    if (caps != capsOn_) {
        capsOn_ = caps;
        emit capsLockChanged(caps);
    }
    if (num != numOn_) {
        numOn_ = num;
        emit numLockChanged(num);
    }
}

} // namespace osk
