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
