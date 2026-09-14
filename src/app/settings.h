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
    bool darkMode = true;
    QString lightTheme = QStringLiteral("default");
    QString darkTheme = QStringLiteral("win10-dark");
    bool showFrow = false;
    bool showNumpad = false;
    bool showKana = true;
    bool showIndicators = true;
    int keyUnit = 72; // absolute key size in px; 0 = use the theme's key_unit
    QString layoutId = QStringLiteral("us");
    QString modeId = QStringLiteral("full");

    // The theme of the mode that is currently on.
    QString themeId() const { return darkMode ? darkTheme : lightTheme; }
    void setThemeId(const QString &id) { (darkMode ? darkTheme : lightTheme) = id; }

    QPoint positionFor(const QString &screen) const;
    void setPosition(const QString &screen, const QPoint &pos);

private:
    QHash<QString, QPoint> positions_;
};

} // namespace osk
