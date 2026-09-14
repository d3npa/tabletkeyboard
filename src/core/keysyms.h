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

// The five sticky modifiers that are pressed through X, in the order they are
// pressed.
QStringList modifierIds();
// Every modifier the OSK knows, including "fn", which changes the keysym a
// later key sends but has no X keysym of its own.
QStringList allModifierIds();
bool isModifierId(const QString &mod);
QString modifierKeysymName(const QString &mod); // empty for unknown ids and "fn"

// Best-effort printable character for a keysym (Latin-1 and Unicode planes),
// used for shifted hints on keys. Empty when there is no sensible character.
QString displayTextForKeysym(quint32 keysym);

} // namespace osk
