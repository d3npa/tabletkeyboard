#include "app/autostart.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTextStream>

namespace osk {

QString Autostart::filePath()
{
    return QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)
            + QStringLiteral("/autostart/tabletkeyboard.desktop");
}

bool Autostart::isEnabled()
{
    return QFile::exists(filePath());
}

bool Autostart::setEnabled(bool on, QString *error)
{
    const QString path = filePath();

    if (!on) {
        if (QFile::exists(path) && !QFile::remove(path)) {
            if (error)
                *error = QStringLiteral("cannot remove %1").arg(path);
            return false;
        }
        return true;
    }

    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        if (error)
            *error = QStringLiteral("cannot write %1: %2").arg(path, file.errorString());
        return false;
    }

    QTextStream out(&file);
    out << "[Desktop Entry]\n"
        << "Type=Application\n"
        << "Name=Tablet Keyboard\n"
        << "Comment=On-screen keyboard for X11\n"
        << "Exec=" << QCoreApplication::applicationFilePath() << "\n"
        << "Icon=tabletkeyboard\n"
        << "Terminal=false\n"
        << "X-GNOME-Autostart-enabled=true\n";
    file.close();
    return true;
}

} // namespace osk
