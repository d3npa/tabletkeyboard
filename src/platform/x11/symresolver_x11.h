#pragma once

#include "core/sym_resolver.h"

namespace osk {

// Keysym name resolution through libX11's keysym tables. Needs no connection:
// XStringToKeysym() is a pure table lookup.
class XlibKeysymResolver : public KeysymResolver
{
public:
    bool fromName(const QString &name, quint32 *keysym) const override;
};

} // namespace osk
