#pragma once

#include "core/action.h"
#include "platform/input_backend.h"
#include "platform/x11/keycodes.h"

#include <QHash>
#include <QSet>
#include <QString>

namespace osk {
namespace x11 {

// XTEST-based injection. Fake key events enter the X server's normal input
// path, so toolkits, XIM and fcitx5 see exactly what a hardware keyboard
// produces. XSendEvent must never be used for this.
class XTestInputBackend : public InputBackend
{
public:
    XTestInputBackend(Display *dpy, bool available, const QString &unavailableReason);

    bool available() const override { return available_; }
    QString unavailableReason() const override { return reason_; }
    bool execute(const KeyScript &script) override;
    void syncKeymap() override;
    void shutdown() override;

private:
    enum Family { NoFamily = -1, ShiftFamily = 0, CtrlFamily, AltFamily, SuperFamily, AltGrFamily };

    static int familyOf(quint32 keysym);
    static quint32 familyKeysym(int family);

    void pressKeysym(quint32 keysym);
    void releaseKeysym(quint32 keysym);
    void tapKeysym(quint32 keysym);
    void pressFamily(int family);
    void releaseFamily(int family);
    bool fakeKey(KeyCode keycode, bool press);

    Display *dpy_;
    bool available_;
    QString reason_;
    KeycodeAllocator allocator_;
    QSet<int> heldFamilies_; // modifier families we currently hold down
};

} // namespace x11
} // namespace osk
