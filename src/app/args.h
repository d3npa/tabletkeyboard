// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 d3npa <gh@w1t.ch>
#pragma once

#include <QSet>
#include <QString>
#include <QStringList>

namespace osk {

// The command line understood by tabletkeyboard, whether given directly or
// forwarded by a second instance. Values are validated by parseCommandLine();
// unset options keep their "not given" defaults.
struct CommandLineOptions
{
    bool show = false;
    bool hide = false;
    bool toggle = false;
    bool dark = false;
    bool light = false;
    QString mode;
    QString language;
    QString theme;
    QSet<QString> blocks; // block ids given to --blocks
    double scale = 0.0;   // 0 = not given
};

// Parses CLI arguments. Returns false and sets `error` for an unknown option,
// a missing value or an invalid value; the caller reports it and exits
// non-zero.
bool parseCommandLine(const QStringList &args, CommandLineOptions *out, QString *error);

} // namespace osk
