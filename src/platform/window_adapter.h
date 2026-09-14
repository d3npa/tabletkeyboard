// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 d3npa <gh@w1t.ch>
#pragma once

#include <QtGlobal>

namespace osk {

// Platform window behaviour that Qt's cross-platform flags cannot express:
// EWMH state, sticky-across-desktops, and freeing the OSK from focus duties.
class WindowAdapter
{
public:
    virtual ~WindowAdapter() = default;

    // Called once the native window exists and is mapped.
    virtual void configure(quintptr windowId) = 0;

    // Toggle "show on every virtual desktop" (EWMH _NET_WM_DESKTOP).
    virtual void setOnAllDesktops(quintptr windowId, bool on) = 0;
};

} // namespace osk
