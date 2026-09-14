// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 d3npa <gh@w1t.ch>
#include "platform/x11/window_x11.h"

#include <X11/Xatom.h>
#include <X11/Xlib.h>

namespace osk {
namespace x11 {

X11WindowAdapter::X11WindowAdapter(Display *dpy, bool allDesktops) : dpy_(dpy), allDesktops_(allDesktops) {}

void X11WindowAdapter::configure(quintptr windowId)
{
    if (!dpy_ || !windowId)
        return;
    const Window window = Window(windowId);

    // Qt adds _KDE_NET_WM_WINDOW_TYPE_OVERRIDE to tool windows; KWin then stops
    // applying desktop handling to us, so keep only the plain EWMH utility type.
    const Atom windowType = XInternAtom(dpy_, "_NET_WM_WINDOW_TYPE", False);
    const Atom utility = XInternAtom(dpy_, "_NET_WM_WINDOW_TYPE_UTILITY", False);
    XChangeProperty(dpy_, window, windowType, XA_ATOM, 32, PropModeReplace,
                    reinterpret_cast<const unsigned char *>(&utility), 1);

    setOnAllDesktops(windowId, allDesktops_);
    addNetWmStateProperty(window, { QStringLiteral("_NET_WM_STATE_SKIP_TASKBAR"),
                                    QStringLiteral("_NET_WM_STATE_SKIP_PAGER") });

    // The property covers WMs that read it at manage time; the client message
    // is the EWMH path for WMs that are already managing us.
    sendNetWmStates(window, { QStringLiteral("_NET_WM_STATE_SKIP_TASKBAR"),
                              QStringLiteral("_NET_WM_STATE_SKIP_PAGER") },
                    true);
    XFlush(dpy_);
}

void X11WindowAdapter::setOnAllDesktops(quintptr windowId, bool on)
{
    if (!dpy_ || !windowId)
        return;
    allDesktops_ = on;

    const Window window = Window(windowId);
    const Atom netWmDesktop = XInternAtom(dpy_, "_NET_WM_DESKTOP", False);
    const long value = on ? 0xFFFFFFFFL : 0L;
    XChangeProperty(dpy_, window, netWmDesktop, XA_CARDINAL, 32, PropModeReplace,
                    reinterpret_cast<const unsigned char *>(&value), 1);

    // The property is what a WM reads when it starts managing the window; the
    // client messages are the EWMH requests that also reach a running WM.
    // KWin tracks "on all desktops" as _NET_WM_STATE_STICKY and ignores later
    // property writes, so both channels are needed.
    XEvent event{};
    event.xclient.type = ClientMessage;
    event.xclient.window = window;
    event.xclient.message_type = netWmDesktop;
    event.xclient.format = 32;
    event.xclient.data.l[0] = value;
    event.xclient.data.l[1] = 1; // source indication: normal application
    XSendEvent(dpy_, DefaultRootWindow(dpy_), False,
               SubstructureRedirectMask | SubstructureNotifyMask, &event);

    sendNetWmStates(window, { QStringLiteral("_NET_WM_STATE_STICKY") }, on);
    XFlush(dpy_);
}

// Sends one _NET_WM_STATE client message per pair of atoms (the message format
// carries two state atoms).
void X11WindowAdapter::sendNetWmStates(Window window, const QVector<QString> &states, bool add)
{
    for (int i = 0; i < states.size(); i += 2) {
        const QString &first = states.at(i);
        const QString second = (i + 1 < states.size()) ? states.at(i + 1) : QString();
        sendNetWmStateMessage(window, first, second, add);
    }
}

void X11WindowAdapter::addNetWmStateProperty(Window window, const QVector<QString> &states)
{
    const Atom netWmState = XInternAtom(dpy_, "_NET_WM_STATE", False);

    QVector<Atom> atoms;
    Atom actualType = None;
    int actualFormat = 0;
    unsigned long count = 0;
    unsigned long bytesAfter = 0;
    unsigned char *data = nullptr;
    if (XGetWindowProperty(dpy_, window, netWmState, 0, 1024, False, XA_ATOM, &actualType, &actualFormat,
                           &count, &bytesAfter, &data)
            == Success
        && data) {
        const Atom *list = reinterpret_cast<const Atom *>(data);
        for (unsigned long i = 0; i < count; ++i)
            atoms.append(list[i]);
        XFree(data);
    }

    for (const QString &state : states) {
        const Atom atom = XInternAtom(dpy_, state.toLatin1().constData(), False);
        if (!atoms.contains(atom))
            atoms.append(atom);
    }

    XChangeProperty(dpy_, window, netWmState, XA_ATOM, 32, PropModeReplace,
                    reinterpret_cast<const unsigned char *>(atoms.constData()), atoms.size());
}

void X11WindowAdapter::sendNetWmStateMessage(Window window, const QString &state1, const QString &state2,
                                             bool add)
{
    XEvent event{};
    event.xclient.type = ClientMessage;
    event.xclient.window = window;
    event.xclient.message_type = XInternAtom(dpy_, "_NET_WM_STATE", False);
    event.xclient.format = 32;
    event.xclient.data.l[0] = add ? 1 : 0; // _NET_WM_STATE_ADD / _REMOVE
    event.xclient.data.l[1] = long(XInternAtom(dpy_, state1.toLatin1().constData(), False));
    event.xclient.data.l[2] = state2.isEmpty() ? 0 : long(XInternAtom(dpy_, state2.toLatin1().constData(), False));
    event.xclient.data.l[3] = 1; // source indication: normal application

    XSendEvent(dpy_, DefaultRootWindow(dpy_), False,
               SubstructureRedirectMask | SubstructureNotifyMask, &event);
}

} // namespace x11
} // namespace osk
