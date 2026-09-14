// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 d3npa <gh@w1t.ch>
#pragma once

#include "platform/window_adapter.h"

#include <QString>
#include <QVector>

// Forward declarations only; see xconnection.h for why Xlib.h stays out of
// headers here.
typedef struct _XDisplay Display;
typedef unsigned long Window;

namespace osk {
namespace x11 {

// EWMH behaviour Qt does not expose: keep the OSK out of the taskbar/pager
// and show it on every virtual desktop.
class X11WindowAdapter : public WindowAdapter
{
public:
    X11WindowAdapter(Display *dpy, bool allDesktops);

    void configure(quintptr windowId) override;
    void setOnAllDesktops(quintptr windowId, bool on) override;

private:
    void addNetWmStateProperty(Window window, const QVector<QString> &states);
    void sendNetWmStates(Window window, const QVector<QString> &states, bool add);
    void sendNetWmStateMessage(Window window, const QString &state1, const QString &state2, bool add);

    Display *dpy_;
    bool allDesktops_;
};

} // namespace x11
} // namespace osk
