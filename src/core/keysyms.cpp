#include "core/keysyms.h"

namespace osk {

QStringList modifierIds()
{
    return { QStringLiteral("ctrl"),  QStringLiteral("shift"), QStringLiteral("alt"),
             QStringLiteral("super"), QStringLiteral("altgr") };
}

bool isModifierId(const QString &mod)
{
    return modifierIds().contains(mod);
}

QString modifierKeysymName(const QString &mod)
{
    if (mod == QLatin1String("shift"))
        return QLatin1String(modsym::kShift);
    if (mod == QLatin1String("ctrl"))
        return QLatin1String(modsym::kCtrl);
    if (mod == QLatin1String("alt"))
        return QLatin1String(modsym::kAlt);
    if (mod == QLatin1String("super"))
        return QLatin1String(modsym::kSuper);
    if (mod == QLatin1String("altgr"))
        return QLatin1String(modsym::kAltGr);
    return QString();
}

QString displayTextForKeysym(quint32 keysym)
{
    if (keysym == 0)
        return QString();
    if (keysym == 0x20ac)
        return QStringLiteral("€");
    if (keysym >= 0x20 && keysym <= 0x7e)
        return QString(QChar(static_cast<char>(keysym)));
    if (keysym >= 0xa0 && keysym <= 0xff)
        return QString(QChar(static_cast<char>(keysym)));
    if (keysym >= 0x01000000 && keysym <= 0x0010ffff) {
        const uint cp = keysym - 0x01000000;
        if (cp == 0)
            return QString();
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        const char32_t c = static_cast<char32_t>(cp);
#else
        const uint c = cp;
#endif
        return QString::fromUcs4(&c, 1);
    }
    return QString();
}

} // namespace osk
