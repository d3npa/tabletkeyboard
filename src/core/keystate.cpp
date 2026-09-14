// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 d3npa <gh@w1t.ch>
#include "core/keystate.h"
#include "core/keysyms.h"
#include "core/sym_resolver.h"

namespace osk {

namespace {

// KeyDefs travel by value, so identify a key by what it would send; used to
// tell "the repeating key was released" from "some other key was released".
bool isSameKey(const KeyDef &a, const KeyDef &b)
{
    return a.type == b.type && a.symCode == b.symCode && a.shiftedCode == b.shiftedCode
            && a.fnCode == b.fnCode;
}

} // namespace

KeyStateMachine::KeyStateMachine(const KeysymResolver *resolver, const LayoutLibrary *library, QObject *parent)
    : QObject(parent), resolver_(resolver), library_(library)
{
    repeatTimer_ = new QTimer(this);
    repeatTimer_->setSingleShot(false);
    connect(repeatTimer_, &QTimer::timeout, this, &KeyStateMachine::repeatTick);

    for (const QString &mod : allModifierIds()) {
        quint32 keysym = 0;
        if (resolver_)
            resolver_->fromName(modifierKeysymName(mod), &keysym);
        if (keysym != 0)
            modKeysyms_.insert(mod, keysym);
    }

    if (library_ && !library_->sets().isEmpty()) {
        layoutId_ = library_->sets().first().id;
        if (const Mode *m = library_->sets().first().primaryMode())
            modeId_ = m->name;
    }
}

KeyStateMachine::~KeyStateMachine()
{
    qDeleteAll(mods_);
}

const LayoutSet *KeyStateMachine::layout() const
{
    return library_ ? library_->byId(layoutId_) : nullptr;
}

const Mode *KeyStateMachine::mode() const
{
    const LayoutSet *set = layout();
    return set ? set->mode(modeId_) : nullptr;
}

const Layer *KeyStateMachine::layer() const
{
    const Mode *m = mode();
    if (!m)
        return nullptr;
    if (layerId_.isEmpty())
        return m->primaryLayer();
    return m->layer(layerId_);
}

QString KeyStateMachine::layerId() const
{
    const Layer *l = layer();
    return l ? l->name : QString();
}

bool KeyStateMachine::applyLayout(const QString &id)
{
    if (!library_)
        return false;
    const LayoutSet *set = library_->byId(id);
    if (!set || layoutId_ == id)
        return false;
    layoutId_ = id;
    if (!set->mode(modeId_)) {
        const Mode *primary = set->primaryMode();
        modeId_ = primary ? primary->name : QString();
    }
    layerId_.clear();
    return true;
}

void KeyStateMachine::setLayoutId(const QString &id)
{
    if (!applyLayout(id))
        return;
    Output out;
    out.setLanguage = layoutId_;
    out.viewChanged = true;
    out.stateChanged = true;
    emit output(out);
}

void KeyStateMachine::setMode(const QString &name)
{
    const LayoutSet *set = layout();
    if (!set || !set->mode(name) || modeId_ == name)
        return;
    modeId_ = name;
    layerId_.clear();
    Output out;
    out.setMode = modeId_;
    out.viewChanged = true;
    out.stateChanged = true;
    emit output(out);
}

bool KeyStateMachine::setLayerInternal(const QString &name)
{
    const Mode *m = mode();
    if (!m || !m->layer(name) || layerId_ == name)
        return false;
    layerId_ = name;
    return true;
}

void KeyStateMachine::setLayer(const QString &name)
{
    if (!setLayerInternal(name))
        return;
    Output out;
    out.setLayer = layerId();
    out.viewChanged = true;
    out.stateChanged = true;
    emit output(out);
}

void KeyStateMachine::cycleLayout()
{
    Output out;
    cycleLayout(&out);
    if (!out.isEmpty())
        emit output(out);
}

void KeyStateMachine::cycleLayout(Output *out)
{
    if (!library_ || library_->sets().size() < 2)
        return;
    const int count = library_->sets().size();
    const int current = qMax(0, library_->indexOf(layoutId_));
    if (!applyLayout(library_->sets().at((current + 1) % count).id))
        return;
    out->setLanguage = layoutId_;
    out->viewChanged = true;
    out->stateChanged = true;
    if (zenkakuOnLangSwitch_ && resolver_) {
        quint32 keysym = 0;
        if (resolver_->fromName(QLatin1String("Zenkaku_Hankaku"), &keysym) && keysym != 0)
            out->script.append(KeyAction(KeyAction::Tap, keysym));
    }
}

void KeyStateMachine::toggleMode()
{
    Output out;
    cycleMode(&out);
    if (!out.isEmpty())
        emit output(out);
}

void KeyStateMachine::cycleMode(Output *out)
{
    const LayoutSet *set = layout();
    if (!set || set->modes.size() < 2)
        return;
    int index = 0;
    for (int i = 0; i < set->modes.size(); ++i) {
        if (set->modes.at(i).name == modeId_) {
            index = i;
            break;
        }
    }
    modeId_ = set->modes.at((index + 1) % set->modes.size()).name;
    layerId_.clear();
    out->setMode = modeId_;
    out->viewChanged = true;
    out->stateChanged = true;
}

void KeyStateMachine::doAction(const KeyDef &key, Output *out)
{
    if (key.action == QLatin1String("hide"))
        out->hide = true;
    else if (key.action == QLatin1String("toggle_mode"))
        cycleMode(out);
    else if (key.action == QLatin1String("toggle_lang"))
        cycleLayout(out);
    else if (key.action == QLatin1String("layer")) {
        if (setLayerInternal(key.layer)) {
            out->setLayer = layerId();
            out->viewChanged = true;
            out->stateChanged = true;
        }
    }
}

void KeyStateMachine::press(const KeyDef &key)
{
    switch (key.type) {
    case KeyDef::Spacer:
        return;
    case KeyDef::Mod:
        pressMod(key.mod);
        return;
    case KeyDef::Action: {
        Output out;
        doAction(key, &out);
        if (!out.isEmpty())
            emit output(out);
        return;
    }
    case KeyDef::Key:
        break;
    }

    Output out = buildKeyTap(key);
    if (key.repeat)
        startRepeat(key);
    emit output(out);
}

void KeyStateMachine::release(const KeyDef &key)
{
    switch (key.type) {
    case KeyDef::Mod:
        releaseMod(key.mod);
        break;
    case KeyDef::Key:
        // Only the key that is repeating may stop its repeat: with two keys
        // held, releasing the other one must not cancel it.
        if (isSameKey(key, repeatKey_))
            stopRepeat();
        break;
    default:
        break;
    }
}

Output KeyStateMachine::buildKeyTap(const KeyDef &key)
{
    Output out;

    QStringList active; // every active modifier, "fn" included
    for (const QString &mod : allModifierIds()) {
        if (modActive(mod))
            active.append(mod);
    }

    // Only modifiers with a resolvable keysym reach the script: a keysym of 0
    // would be a silent no-op in the backend while the UI shows it armed.
    QVector<quint32> scriptMods;
    for (const QString &mod : modifierIds()) {
        if (!active.contains(mod))
            continue;
        const quint32 keysym = modKeysym(mod);
        if (keysym != 0)
            scriptMods.append(keysym);
    }

    for (const quint32 keysym : scriptMods)
        out.script.append(KeyAction(KeyAction::Down, keysym));
    out.script.append(KeyAction(KeyAction::Tap, targetKeysym(key)));
    for (int i = scriptMods.size() - 1; i >= 0; --i)
        out.script.append(KeyAction(KeyAction::Up, scriptMods.at(i)));

    for (const QString &mod : active) {
        ModInfo *info = mods_.value(mod, nullptr);
        if (info && info->state == OneShot) {
            info->state = Off;
            if (info->clearTimer)
                info->clearTimer->stop();
            out.stateChanged = true;
        }
    }
    return out;
}

quint32 KeyStateMachine::targetKeysym(const KeyDef &key) const
{
    if (key.fnCode != 0 && modActive(QLatin1String("fn")))
        return key.fnCode;
    if (key.shiftedCode != 0 && shiftActive())
        return key.shiftedCode;
    return key.symCode;
}

quint32 KeyStateMachine::modKeysym(const QString &mod) const
{
    return modKeysyms_.value(mod, 0);
}

void KeyStateMachine::startRepeat(const KeyDef &key)
{
    stopRepeat();
    repeatKey_ = key;
    repeatStarted_ = false;
    repeatTimer_->start(repeatDelayMs_);
}

void KeyStateMachine::stopRepeat()
{
    repeatTimer_->stop();
    repeatStarted_ = false;
}

void KeyStateMachine::repeatTick()
{
    if (repeatKey_.type != KeyDef::Key) {
        stopRepeat();
        return;
    }
    if (!repeatStarted_) {
        repeatStarted_ = true;
        repeatTimer_->setInterval(repeatIntervalMs_);
    }
    emit output(buildKeyTap(repeatKey_));
}

KeyStateMachine::ModInfo *KeyStateMachine::modInfo(const QString &mod)
{
    auto it = mods_.find(mod);
    if (it == mods_.end()) {
        auto *info = new ModInfo;
        info->holdTimer = new QTimer(this);
        info->holdTimer->setSingleShot(true);
        info->clearTimer = new QTimer(this);
        info->clearTimer->setSingleShot(true);
        connect(info->holdTimer, &QTimer::timeout, this, [this, mod]() { longPressMod(mod); });
        connect(info->clearTimer, &QTimer::timeout, this, [this, mod]() { expireSticky(mod); });
        it = mods_.insert(mod, info);
    }
    return it.value();
}

void KeyStateMachine::pressMod(const QString &mod)
{
    ModInfo *info = modInfo(mod);
    info->longPressFired = false;
    info->holdTimer->start(longPressMs_);
}

void KeyStateMachine::releaseMod(const QString &mod)
{
    ModInfo *info = modInfo(mod);
    info->holdTimer->stop();
    if (info->longPressFired) {
        info->longPressFired = false;
        return; // the long press already toggled the lock
    }

    switch (info->state) {
    case Off:
        info->state = OneShot;
        if (stickyTimeoutMs_ > 0)
            info->clearTimer->start(stickyTimeoutMs_);
        break;
    case OneShot:
        info->state = Locked;
        info->clearTimer->stop();
        break;
    case Locked:
        info->state = Off;
        break;
    }

    Output out;
    out.stateChanged = true;
    emit output(out);
}

void KeyStateMachine::longPressMod(const QString &mod)
{
    ModInfo *info = modInfo(mod);
    info->longPressFired = true;
    info->state = (info->state == Locked) ? Off : Locked;
    info->clearTimer->stop();

    Output out;
    out.stateChanged = true;
    emit output(out);
}

void KeyStateMachine::expireSticky(const QString &mod)
{
    ModInfo *info = modInfo(mod);
    if (info->state != OneShot)
        return;
    info->state = Off;

    Output out;
    out.stateChanged = true;
    emit output(out);
}

KeyStateMachine::ModState KeyStateMachine::modState(const QString &mod) const
{
    ModInfo *info = mods_.value(mod, nullptr);
    return info ? info->state : Off;
}

bool KeyStateMachine::modActive(const QString &mod) const
{
    return modState(mod) != Off;
}

bool KeyStateMachine::shiftActive() const
{
    return modActive(QLatin1String("shift"));
}

void KeyStateMachine::setCapsOn(bool on)
{
    if (capsOn_ == on)
        return;
    capsOn_ = on;
    Output out;
    out.stateChanged = true;
    emit output(out);
}

void KeyStateMachine::setNumOn(bool on)
{
    if (numOn_ == on)
        return;
    numOn_ = on;
    Output out;
    out.stateChanged = true;
    emit output(out);
}

void KeyStateMachine::setScrollOn(bool on)
{
    if (scrollOn_ == on)
        return;
    scrollOn_ = on;
    Output out;
    out.stateChanged = true;
    emit output(out);
}

void KeyStateMachine::setRepeatTiming(int delayMs, int intervalMs)
{
    repeatDelayMs_ = qMax(1, delayMs);
    repeatIntervalMs_ = qMax(1, intervalMs);
}

} // namespace osk
