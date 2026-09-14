// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 d3npa <gh@w1t.ch>
#pragma once

#include <QMetaType>
#include <QString>
#include <QVector>
#include <QtGlobal>

namespace osk {

// One primitive step of a key script, expressed in keysyms. The platform
// backend resolves keysyms to keycodes and synthesizes whatever modifiers are
// needed; this is the only representation that leaves osk-core.
struct KeyAction
{
    enum Type { Tap, Down, Up };

    Type type = Tap;
    quint32 keysym = 0;

    KeyAction() = default;
    KeyAction(Type t, quint32 ks) : type(t), keysym(ks) {}
};

using KeyScript = QVector<KeyAction>;

// Everything one user interaction can produce: key events to inject, view
// changes to apply, or a request to hide the window.
struct Output
{
    KeyScript script;
    bool hide = false;
    QString setMode;
    QString setLayer;
    QString setLanguage;
    bool viewChanged = false;  // mode/layer/language changed: rebuild the key grid
    bool stateChanged = false; // sticky/caps visuals changed: repaint

    bool isEmpty() const
    {
        return script.isEmpty() && !hide && setMode.isEmpty() && setLayer.isEmpty()
                && setLanguage.isEmpty() && !viewChanged && !stateChanged;
    }
};

} // namespace osk

Q_DECLARE_METATYPE(osk::KeyScript)
Q_DECLARE_METATYPE(osk::Output)
