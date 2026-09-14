#pragma once

#include "core/action.h"

#include <QString>

namespace osk {

// Keeps osk-core free of X11: the app talks to the input backend through this
// interface, so a Wayland backend can be added without touching the core.
class InputBackend
{
public:
    virtual ~InputBackend() = default;

    virtual bool available() const = 0;
    virtual QString unavailableReason() const = 0;

    // Execute one script. Events are delivered to whatever holds the X input
    // focus; the OSK window itself never has it.
    virtual bool execute(const KeyScript &script) = 0;

    // The keyboard mapping changed under us (XkbMapNotify).
    virtual void syncKeymap() {}

    // Restore anything that was remapped.
    virtual void shutdown() {}
};

} // namespace osk
