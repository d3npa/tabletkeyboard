// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 d3npa <gh@w1t.ch>
#include "app/settings.h"

#include <QSettings>

namespace osk {

namespace {

constexpr const char *kPositionGroup = "position";
// QSettings reserves the plain name "General" for the top-level section (the
// file's "[General]", which these settings do not use), so the group is
// escaped to "[%General]" in the file either way. Reading it back yields the
// group "General" (capital G) even when it was written as "general", so both
// the reader and the writer use "General/..." and the file keeps one section.
constexpr const char *kGeneralGroup = "General";

QString generalKey(const char *key)
{
    return QLatin1String(kGeneralGroup) + QLatin1Char('/') + QLatin1String(key);
}

// Accepts every spelling: "General/*" (this app), a hand-written "[general]"
// section, and the old top-level keys.
QVariant readGeneral(const QSettings &settings, const char *key, const QVariant &fallback)
{
    for (const QString &candidate : { generalKey(key), QStringLiteral("general/") + QLatin1String(key) }) {
        const QVariant value = settings.value(candidate);
        if (value.isValid())
            return value;
    }
    const QVariant top = settings.value(QLatin1String(key));
    return top.isValid() ? top : fallback;
}

} // namespace

void AppSettings::load()
{
    QSettings settings;
    scale = readGeneral(settings, "scale", 1.0).toDouble();
    stickyTimeoutMs = readGeneral(settings, "stickyTimeoutMs", 0).toInt();
    zenkakuOnLangSwitch = readGeneral(settings, "zenkakuOnLangSwitch", false).toBool();
    startAtLogin = readGeneral(settings, "startAtLogin", false).toBool();
    onAllDesktops = readGeneral(settings, "onAllDesktops", true).toBool();
    darkMode = settings.value(QStringLiteral("theme/darkMode"), true).toBool();
    lightTheme = settings.value(QStringLiteral("theme/lightTheme"), QStringLiteral("default")).toString();
    darkTheme = settings.value(QStringLiteral("theme/darkTheme"), QStringLiteral("default-dark")).toString();
    showFrow = settings.value(QStringLiteral("blocks/frow"), false).toBool();
    showNumpad = settings.value(QStringLiteral("blocks/numpad"), false).toBool();
    showKana = settings.value(QStringLiteral("keys/kana"), true).toBool();
    showIndicators = settings.value(QStringLiteral("keys/indicators"), true).toBool();
    keyUnit = readGeneral(settings, "keyUnit", 72).toInt();
    layoutId = readGeneral(settings, "layout", QStringLiteral("us")).toString();
    modeId = readGeneral(settings, "mode", QStringLiteral("full")).toString();

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
    settings.setValue(generalKey("scale"), scale);
    settings.setValue(generalKey("stickyTimeoutMs"), stickyTimeoutMs);
    settings.setValue(generalKey("zenkakuOnLangSwitch"), zenkakuOnLangSwitch);
    settings.setValue(generalKey("startAtLogin"), startAtLogin);
    settings.setValue(generalKey("onAllDesktops"), onAllDesktops);
    settings.setValue(QStringLiteral("theme/darkMode"), darkMode);
    settings.setValue(QStringLiteral("theme/lightTheme"), lightTheme);
    settings.setValue(QStringLiteral("theme/darkTheme"), darkTheme);
    settings.setValue(QStringLiteral("blocks/frow"), showFrow);
    settings.setValue(QStringLiteral("blocks/numpad"), showNumpad);
    settings.setValue(QStringLiteral("keys/kana"), showKana);
    settings.setValue(QStringLiteral("keys/indicators"), showIndicators);
    settings.setValue(generalKey("keyUnit"), keyUnit);
    settings.setValue(generalKey("layout"), layoutId);
    settings.setValue(generalKey("mode"), modeId);

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
