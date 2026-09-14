// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 d3npa <gh@w1t.ch>
#include "platform/x11/symresolver_x11.h"

#include <X11/Xlib.h>
#include <X11/keysym.h>

namespace osk {

bool XlibKeysymResolver::fromName(const QString &name, quint32 *keysym) const
{
    if (name.isEmpty())
        return false;
    const QByteArray latin = name.toLatin1();
    const KeySym resolved = XStringToKeysym(latin.constData());
    if (resolved == NoSymbol)
        return false;
    if (keysym)
        *keysym = quint32(resolved);
    return true;
}

} // namespace osk
