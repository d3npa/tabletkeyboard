// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 d3npa <gh@w1t.ch>
#pragma once

#include <QString>

namespace osk {

// KDE/GNOME-compatible autostart entry in ~/.config/autostart.
class Autostart
{
public:
    static QString filePath();
    static bool isEnabled();
    static bool setEnabled(bool on, QString *error = nullptr);
};

} // namespace osk
