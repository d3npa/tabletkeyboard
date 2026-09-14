#pragma once

#include <QHash>
#include <QSet>
#include <QVector>

// Forward declarations only; see xconnection.h for why Xlib.h stays out of
// headers here. These typedefs match Xlib exactly.
typedef struct _XDisplay Display;
typedef unsigned long KeySym;
typedef unsigned char KeyCode;

namespace osk {
namespace x11 {

// A natural position of a keysym in the current core keyboard map.
struct KeyLocation
{
    KeyCode keycode = 0;
    int level = 0; // 0 = base, 1 = shift, 2 = altgr, 3 = shift+altgr
};

// Owns the keyboard mapping view: finds natural keycodes for keysyms and keeps
// a pool of spare keycodes remapped on demand (the xdotool technique, kept
// warm so nothing is remapped while a key is in flight). Spares are restored
// on exit and re-created after an external map reset (XkbMapNotify).
class KeycodeAllocator
{
public:
    explicit KeycodeAllocator(Display *dpy);

    // (Re)read the keyboard mapping; drops all caches and spare keycodes.
    void refresh();

    // Look up a keycode/level that produces keysym in the current map.
    bool findNatural(quint32 keysym, KeyLocation *out);

    // Keycode that produces keysym regardless of modifier state. 0 on failure.
    KeyCode ensureSpare(quint32 keysym);
    KeyCode spareFor(quint32 keysym) const { return sparesFor_.value(keysym, 0); }

    // Keycode that acts as a modifier (spare when possible, so releasing it
    // can never clobber a physically held key).
    KeyCode modifierKeycode(quint32 modKeysym);

    void restoreSpares();

private:
    KeySym keysymAt(KeyCode keycode, int level) const;
    bool keycodeIsFree(KeyCode keycode) const;
    bool keycodeHasKeysymLive(KeyCode keycode, KeySym keysym) const;
    void writeKeycode(KeyCode keycode, KeySym keysym);
    bool probeSpareModifiers();

    Display *dpy_;
    KeyCode minKeycode_ = 0;
    KeyCode maxKeycode_ = 0;
    int keysymsPerKeycode_ = 0;
    QVector<KeySym> map_;
    QVector<KeyCode> freeKeycodes_;
    QHash<quint32, KeyLocation> naturalCache_;
    QHash<quint32, KeyCode> sparesFor_;
    QSet<KeyCode> spares_;
    QHash<quint32, KeyCode> modifierKeycodes_;
    bool modifierProbeDone_ = false;
    bool spareModifiers_ = false;
    bool hasAltGr_ = false;
};

} // namespace x11
} // namespace osk
