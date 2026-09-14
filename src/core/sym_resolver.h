// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 d3npa <gh@w1t.ch>
#pragma once

#include <QString>
#include <QtGlobal>

namespace osk {

// Keysym name <-> code resolution. Implemented by the platform layer (Xlib)
// so that osk-core stays free of X11.
class KeysymResolver
{
public:
    virtual ~KeysymResolver() = default;

    // Resolve an X keysym name ("Henkan", "a", "BackSpace") to a keysym code.
    // Returns false for unknown names.
    virtual bool fromName(const QString &name, quint32 *keysym) const = 0;
};

} // namespace osk
