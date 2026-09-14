#pragma once

#include <QObject>
#include <QString>

// X11 types are forward declared here on purpose: including Xlib.h from a
// header that moc sees injects unprefixed macros (Bool, None, Status,
// CursorShape, ...) that collide with Qt names. Only the .cpp files include
// the real Xlib headers, and they do so after all Qt headers.
typedef struct _XDisplay Display;

class QSocketNotifier;

namespace osk {

// Owns a private Xlib connection (separate from Qt's) used for key injection,
// XKB monitoring and EWMH property writes. Events are pumped through a
// QSocketNotifier on the main thread: no polling, no idle wakeups.
class X11Connection : public QObject
{
    Q_OBJECT
public:
    explicit X11Connection(QObject *parent = nullptr);
    ~X11Connection() override;

    bool open(QString *error);
    bool isOpen() const { return display_ != nullptr; }
    Display *display() const { return display_; }

    bool xtestAvailable() const { return xtestOk_; }
    bool capsLockOn() const { return capsOn_; }
    bool numLockOn() const { return numOn_; }
    bool scrollLockOn() const { return scrollOn_; }

signals:
    void capsLockChanged(bool on);
    void numLockChanged(bool on);
    void scrollLockChanged(bool on);
    void keymapChanged();

private:
    void processEvents();
    void readLockMasks();
    void updateLockState(unsigned int lockedMods);

    Display *display_ = nullptr;
    QSocketNotifier *notifier_ = nullptr;
    bool xtestOk_ = false;
    bool xkbOk_ = false;
    int xkbEventBase_ = 0;
    unsigned int numLockMask_ = 0;
    unsigned int scrollLockMask_ = 0;
    bool capsOn_ = false;
    bool numOn_ = false;
    bool scrollOn_ = false;
};

} // namespace osk
