#include "app/settings.h"

#include <QSettings>

namespace osk {

namespace {

constexpr const char *kPositionGroup = "position";

} // namespace

void AppSettings::load()
{
    QSettings settings;
    scale = settings.value(QStringLiteral("general/scale"), 1.0).toDouble();
    stickyTimeoutMs = settings.value(QStringLiteral("general/stickyTimeoutMs"), 0).toInt();
    zenkakuOnLangSwitch = settings.value(QStringLiteral("general/zenkakuOnLangSwitch"), false).toBool();
    startAtLogin = settings.value(QStringLiteral("general/startAtLogin"), false).toBool();
    onAllDesktops = settings.value(QStringLiteral("general/onAllDesktops"), true).toBool();
    darkMode = settings.value(QStringLiteral("theme/darkMode"), true).toBool();
    lightTheme = settings.value(QStringLiteral("theme/lightTheme"), QStringLiteral("win10")).toString();
    darkTheme = settings.value(QStringLiteral("theme/darkTheme"), QStringLiteral("win10-dark")).toString();
    showFrow = settings.value(QStringLiteral("blocks/frow"), false).toBool();
    showNumpad = settings.value(QStringLiteral("blocks/numpad"), false).toBool();
    showKana = settings.value(QStringLiteral("keys/kana"), true).toBool();
    showIndicators = settings.value(QStringLiteral("keys/indicators"), true).toBool();
    // QSettings escapes the "general" group to "[%General]" because top-level
    // keys live in "[General]"; accept a hand-written top-level keyUnit too.
    const QVariant keyUnitInGroup = settings.value(QStringLiteral("general/keyUnit"));
    keyUnit = keyUnitInGroup.isValid() ? keyUnitInGroup.toInt()
                                       : settings.value(QStringLiteral("keyUnit"), 72).toInt();
    layoutId = settings.value(QStringLiteral("general/layout"), QStringLiteral("us")).toString();
    modeId = settings.value(QStringLiteral("general/mode"), QStringLiteral("full")).toString();

    positions_.clear();
    settings.beginGroup(QLatin1String(kPositionGroup));
    const QStringList screens = settings.childKeys();
    for (const QString &screen : screens) {
        const QStringList parts = settings.value(screen).toString().split(QLatin1Char(','));
        if (parts.size() == 2)
            positions_.insert(screen, QPoint(parts.at(0).toInt(), parts.at(1).toInt()));
    }
    settings.endGroup();
}

void AppSettings::save()
{
    QSettings settings;
    settings.setValue(QStringLiteral("general/scale"), scale);
    settings.setValue(QStringLiteral("general/stickyTimeoutMs"), stickyTimeoutMs);
    settings.setValue(QStringLiteral("general/zenkakuOnLangSwitch"), zenkakuOnLangSwitch);
    settings.setValue(QStringLiteral("general/startAtLogin"), startAtLogin);
    settings.setValue(QStringLiteral("general/onAllDesktops"), onAllDesktops);
    settings.setValue(QStringLiteral("theme/darkMode"), darkMode);
    settings.setValue(QStringLiteral("theme/lightTheme"), lightTheme);
    settings.setValue(QStringLiteral("theme/darkTheme"), darkTheme);
    settings.setValue(QStringLiteral("blocks/frow"), showFrow);
    settings.setValue(QStringLiteral("blocks/numpad"), showNumpad);
    settings.setValue(QStringLiteral("keys/kana"), showKana);
    settings.setValue(QStringLiteral("keys/indicators"), showIndicators);
    settings.setValue(QStringLiteral("general/keyUnit"), keyUnit);
    settings.setValue(QStringLiteral("general/layout"), layoutId);
    settings.setValue(QStringLiteral("general/mode"), modeId);

    settings.remove(QLatin1String(kPositionGroup));
    settings.beginGroup(QLatin1String(kPositionGroup));
    for (auto it = positions_.constBegin(); it != positions_.constEnd(); ++it) {
        settings.setValue(it.key(), QStringLiteral("%1,%2").arg(it.value().x()).arg(it.value().y()));
    }
    settings.endGroup();
    settings.sync();
}

QPoint AppSettings::positionFor(const QString &screen) const
{
    return positions_.value(screen, QPoint());
}

void AppSettings::setPosition(const QString &screen, const QPoint &pos)
{
    if (!screen.isEmpty())
        positions_.insert(screen, pos);
}

} // namespace osk
