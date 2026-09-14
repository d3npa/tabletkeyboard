#pragma once

#include "core/sym_resolver.h"

namespace osk {

// Keysym name resolution through libX11's keysym tables. Needs no connection:
// XStringToKeysym()/XKeysymToString() are pure table lookups.
class XlibKeysymResolver : public KeysymResolver
{
public:
    bool fromName(const QString &name, quint32 *keysym) const override;
    QString toName(quint32 keysym) const override;
};

} // namespace osk
