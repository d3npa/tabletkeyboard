#include "platform/x11/symresolver_x11.h"

#include <X11/Xlib.h>
#include <X11/keysym.h>

namespace osk {

bool XlibKeysymResolver::fromName(const QString &name, quint32 *keysym) const
{
    if (name.isEmpty())
        return false;
    const QByteArray latin = name.toLatin1();
    const KeySym resolved = XStringToKeysym(latin.constData());
    if (resolved == NoSymbol)
        return false;
    if (keysym)
        *keysym = quint32(resolved);
    return true;
}

QString XlibKeysymResolver::toName(quint32 keysym) const
{
    if (keysym == 0)
        return QString();
    const char *name = XKeysymToString(KeySym(keysym));
    return name ? QString::fromLatin1(name) : QString();
}

} // namespace osk
