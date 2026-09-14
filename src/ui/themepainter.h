#pragma once

#include "core/theme.h"

#include <QColor>
#include <QString>

class QPainter;
class QRect;

namespace osk {

// Visual state of one key, computed by KeyButton from the state machine.
struct KeyVisual
{
    QString label;
    QString sublabel;
    bool hovered = false;
    bool pressed = false;
    bool active = false; // sticky/locked modifier or lit indicator
    bool locked = false; // locked: drawn with an accent border
};

// Paints keys, the title bar and the window background from a ThemeSpec.
class ThemePainter
{
public:
    explicit ThemePainter(const ThemeSpec &theme);

    const ThemeSpec &theme() const { return theme_; }
    QColor color(const QString &value) const;

    void paintBackground(QPainter &painter, const QRect &rect) const;
    void paintBar(QPainter &painter, const QRect &rect) const;
    // `bodyRect` is the lower part of a stepped key (JIS Return); empty for a
    // plain rectangular key.
    void paintKey(QPainter &painter, const QRect &rect, const QRect &bodyRect, const KeyVisual &visual,
                  double scale) const;

private:
    ThemeSpec theme_;
};

} // namespace osk
