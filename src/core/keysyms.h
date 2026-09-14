#pragma once

#include <QString>
#include <QStringList>
#include <QtGlobal>

namespace osk {

// X keysym names of the modifier keys the OSK can hold.
namespace modsym {
inline constexpr const char *kShift = "Shift_L";
inline constexpr const char *kCtrl = "Control_L";
inline constexpr const char *kAlt = "Alt_L";
inline constexpr const char *kSuper = "Super_L";
inline constexpr const char *kAltGr = "ISO_Level3_Shift";
}

// The five sticky modifiers, in the order they are pressed.
QStringList modifierIds();
bool isModifierId(const QString &mod);
QString modifierKeysymName(const QString &mod); // empty for unknown ids

// Best-effort printable character for a keysym (Latin-1 and Unicode planes),
// used for shifted hints on keys. Empty when there is no sensible character.
QString displayTextForKeysym(quint32 keysym);

} // namespace osk
