#pragma once

#include "core/action.h"
#include "core/layout.h"

#include <QHash>
#include <QObject>
#include <QString>
#include <QTimer>

namespace osk {

class KeysymResolver;

// Sticky-modifier state machine. The UI feeds it key presses and releases;
// it emits key scripts, view changes and hide requests. All decisions about
// sticky/locked modifiers, layers, modes and language switching live here.
class KeyStateMachine : public QObject
{
    Q_OBJECT
public:
    enum ModState { Off, OneShot, Locked };

    KeyStateMachine(const KeysymResolver *resolver, const LayoutLibrary *library, QObject *parent = nullptr);
    ~KeyStateMachine() override;

    // Layout / mode / layer selection. Each emits one Output.
    void setLayoutId(const QString &id);
    void setMode(const QString &name);
    void setLayer(const QString &name);
    void cycleLayout();
    void toggleMode();

    const LayoutSet *layout() const;
    const Mode *mode() const;
    const Layer *layer() const;
    QString layoutId() const { return layoutId_; }
    QString modeId() const { return modeId_; }
    QString layerId() const;

    void press(const KeyDef &key);
    void release(const KeyDef &key);

    ModState modState(const QString &mod) const;
    bool modActive(const QString &mod) const;
    bool shiftActive() const;

    bool capsOn() const { return capsOn_; }
    bool numOn() const { return numOn_; }
    bool scrollOn() const { return scrollOn_; }
    void setCapsOn(bool on);
    void setNumOn(bool on);
    void setScrollOn(bool on);

    void setZenkakuOnLangSwitch(bool on) { zenkakuOnLangSwitch_ = on; }
    void setStickyTimeoutMs(int ms) { stickyTimeoutMs_ = ms; }
    // Test seams: the production values are the member initialisers below
    // (450/55 ms repeat, 450 ms long press); they are not settings.
    void setRepeatTiming(int delayMs, int intervalMs);
    void setLongPressMs(int ms) { longPressMs_ = ms; }

signals:
    void output(const osk::Output &out);

private:
    struct ModInfo
    {
        ModState state = Off;
        bool longPressFired = false;
        QTimer *holdTimer = nullptr;
        QTimer *clearTimer = nullptr;
    };

    ModInfo *modInfo(const QString &mod);
    void pressMod(const QString &mod);
    void releaseMod(const QString &mod);
    void longPressMod(const QString &mod);
    void expireSticky(const QString &mod);
    void repeatTick();

    void doAction(const KeyDef &key, Output *out);
    void cycleMode(Output *out);
    void cycleLayout(Output *out);
    Output buildKeyTap(const KeyDef &key);
    quint32 targetKeysym(const KeyDef &key) const;
    quint32 modKeysym(const QString &mod) const;

    void startRepeat(const KeyDef &key);
    void stopRepeat();

    bool applyLayout(const QString &id);
    bool setLayerInternal(const QString &name);

    const KeysymResolver *resolver_;
    const LayoutLibrary *library_;

    QString layoutId_;
    QString modeId_;
    QString layerId_; // empty = primary layer of the current mode
    QHash<QString, ModInfo *> mods_;
    QHash<QString, quint32> modKeysyms_;
    bool capsOn_ = false;
    bool numOn_ = false;
    bool scrollOn_ = false;
    bool zenkakuOnLangSwitch_ = false;
    int stickyTimeoutMs_ = 0;
    int repeatDelayMs_ = 450;
    int repeatIntervalMs_ = 55;
    int longPressMs_ = 450;
    QTimer *repeatTimer_ = nullptr;
    KeyDef repeatKey_;
    bool repeatStarted_ = false;
};

} // namespace osk
