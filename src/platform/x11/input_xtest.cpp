#include "platform/x11/input_xtest.h"

#include <X11/keysym.h>
#include <X11/extensions/XTest.h>

namespace osk {
namespace x11 {

XTestInputBackend::XTestInputBackend(Display *dpy, bool available, const QString &unavailableReason)
    : dpy_(dpy), available_(available), reason_(unavailableReason), allocator_(dpy)
{
}

int XTestInputBackend::familyOf(quint32 keysym)
{
    switch (keysym) {
    case XK_Shift_L:
    case XK_Shift_R:
        return ShiftFamily;
    case XK_Control_L:
    case XK_Control_R:
        return CtrlFamily;
    case XK_Alt_L:
    case XK_Alt_R:
        return AltFamily;
    case XK_Super_L:
    case XK_Super_R:
        return SuperFamily;
    case XK_ISO_Level3_Shift:
    case XK_Mode_switch:
        return AltGrFamily;
    default:
        return NoFamily;
    }
}

quint32 XTestInputBackend::familyKeysym(int family)
{
    switch (family) {
    case ShiftFamily:
        return XK_Shift_L;
    case CtrlFamily:
        return XK_Control_L;
    case AltFamily:
        return XK_Alt_L;
    case SuperFamily:
        return XK_Super_L;
    case AltGrFamily:
        return XK_ISO_Level3_Shift;
    default:
        return 0;
    }
}

bool XTestInputBackend::fakeKey(KeyCode keycode, bool press)
{
    if (!available_ || !dpy_ || keycode == 0)
        return false;
    return XTestFakeKeyEvent(dpy_, keycode, press ? True : False, 0) != 0;
}

void XTestInputBackend::pressFamily(int family)
{
    if (heldFamilies_.contains(family))
        return;
    const KeyCode keycode = allocator_.modifierKeycode(familyKeysym(family));
    if (!keycode)
        return;
    fakeKey(keycode, true);
    heldFamilies_.insert(family);
}

void XTestInputBackend::releaseFamily(int family)
{
    if (!heldFamilies_.remove(family))
        return;
    const KeyCode keycode = allocator_.modifierKeycode(familyKeysym(family));
    if (keycode)
        fakeKey(keycode, false);
}

void XTestInputBackend::pressKeysym(quint32 keysym)
{
    const int family = familyOf(keysym);
    if (family != NoFamily) {
        pressFamily(family);
        return;
    }
    KeyLocation location;
    if (allocator_.findNatural(keysym, &location)) {
        fakeKey(location.keycode, true);
        return;
    }
    const KeyCode spare = allocator_.ensureSpare(keysym);
    if (spare)
        fakeKey(spare, true);
}

void XTestInputBackend::releaseKeysym(quint32 keysym)
{
    const int family = familyOf(keysym);
    if (family != NoFamily) {
        releaseFamily(family);
        return;
    }
    KeyLocation location;
    const KeyCode keycode = allocator_.findNatural(keysym, &location)
            ? location.keycode
            : allocator_.spareFor(keysym);
    if (keycode)
        fakeKey(keycode, false);
}

void XTestInputBackend::tapKeysym(quint32 keysym)
{
    // A modifier keysym tapped as a key (type "key" in the layout) must not
    // stay down: press and release it. If a script already holds that family,
    // leave it alone instead of clobbering the chord.
    const int family = familyOf(keysym);
    if (family != NoFamily) {
        if (!heldFamilies_.contains(family)) {
            pressFamily(family);
            releaseFamily(family);
        }
        return;
    }

    KeyLocation location;
    if (allocator_.findNatural(keysym, &location)) {
        QVector<int> pressed;
        if (location.level == 1 || location.level == 3) {
            if (!heldFamilies_.contains(ShiftFamily))
                pressed.append(ShiftFamily);
        }
        if (location.level == 2 || location.level == 3) {
            if (!heldFamilies_.contains(AltGrFamily))
                pressed.append(AltGrFamily);
        }
        for (const int pressedFamily : pressed)
            pressFamily(pressedFamily);
        fakeKey(location.keycode, true);
        fakeKey(location.keycode, false);
        for (int i = pressed.size() - 1; i >= 0; --i)
            releaseFamily(pressed.at(i));
        return;
    }

    // Not producible by the current map: use a spare with all levels set to
    // the same keysym, so the active modifier state cannot change the result.
    const KeyCode spare = allocator_.ensureSpare(keysym);
    if (spare) {
        fakeKey(spare, true);
        fakeKey(spare, false);
    }
}

bool XTestInputBackend::execute(const KeyScript &script)
{
    if (!available_ || !dpy_)
        return false;
    for (const KeyAction &action : script) {
        switch (action.type) {
        case KeyAction::Down:
            pressKeysym(action.keysym);
            break;
        case KeyAction::Up:
            releaseKeysym(action.keysym);
            break;
        case KeyAction::Tap:
            tapKeysym(action.keysym);
            break;
        }
    }
    XFlush(dpy_);
    return true;
}

void XTestInputBackend::syncKeymap()
{
    // The map is about to change under us: release anything we still hold
    // first, so a held modifier cannot be left logically down (and the
    // invariant is local to this function, not implied by the callers).
    const QSet<int> held = heldFamilies_;
    for (const int family : held)
        releaseFamily(family);
    allocator_.refresh();
    XFlush(dpy_);
}

void XTestInputBackend::shutdown()
{
    if (dpy_) {
        const QSet<int> held = heldFamilies_;
        for (const int family : held)
            releaseFamily(family);
        XFlush(dpy_);
    }
    allocator_.restoreSpares();
}

} // namespace x11
} // namespace osk
