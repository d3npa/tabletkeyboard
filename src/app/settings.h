#pragma once

#include <QHash>
#include <QPoint>
#include <QString>

namespace osk {

// Configuration persisted at ~/.config/tabletkeyboard/tabletkeyboard.conf.
class AppSettings
{
public:
    void load();
    void save();

    double scale = 1.0;
    int stickyTimeoutMs = 0; // 0 = sticky modifiers never expire on their own
    bool zenkakuOnLangSwitch = false;
    bool startAtLogin = false;
    bool onAllDesktops = true;
    QString themeId = QStringLiteral("win10");
    QString layoutId = QStringLiteral("us");
    QString modeId = QStringLiteral("full");

    QPoint positionFor(const QString &screen) const;
    void setPosition(const QString &screen, const QPoint &pos);

private:
    QHash<QString, QPoint> positions_;
};

} // namespace osk
