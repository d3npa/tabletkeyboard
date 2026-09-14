#include "ui/themepainter.h"

#include <QFontMetricsF>
#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>

namespace osk {

ThemePainter::ThemePainter(const ThemeSpec &theme) : theme_(theme) {}

QColor ThemePainter::color(const QString &value) const
{
    QColor color(value);
    return color.isValid() ? color : QColor(Qt::black);
}

void ThemePainter::paintBackground(QPainter &painter, const QRect &rect) const
{
    painter.fillRect(rect, color(theme_.windowBg));
}

void ThemePainter::paintBar(QPainter &painter, const QRect &rect) const
{
    painter.fillRect(rect, color(theme_.barBg));
}

void ThemePainter::paintKey(QPainter &painter, const QRect &rect, const QRect &bodyRect,
                            const KeyVisual &visual, double scale) const
{
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);

    QColor top;
    QColor bottom;
    if (visual.active) {
        top = color(theme_.modActive);
        bottom = color(theme_.modActive).darker(105);
    } else if (visual.pressed) {
        top = color(theme_.keyPressedTop);
        bottom = color(theme_.keyPressedBottom);
    } else if (visual.hovered) {
        top = color(theme_.keyHover);
        bottom = color(theme_.keyHover).darker(104);
    } else {
        top = color(theme_.keyTop);
        bottom = color(theme_.keyBottom);
    }

    QLinearGradient gradient(rect.topLeft(), rect.bottomLeft());
    gradient.setColorAt(0.0, top);
    gradient.setColorAt(1.0, bottom);

    const qreal borderWidth = qMax(1, qRound(theme_.border * scale));
    QPen pen(visual.locked ? color(theme_.accent) : color(theme_.keyBorder));
    pen.setWidthF(borderWidth);

    const QRectF keyRect = QRectF(rect).adjusted(borderWidth / 2.0, borderWidth / 2.0,
                                                -borderWidth / 2.0, -borderWidth / 2.0);
    const qreal radius = theme_.radius * scale;
    painter.setPen(pen);
    painter.setBrush(gradient);

    QRectF textRect;
    if (bodyRect.isEmpty()) {
        painter.drawRoundedRect(keyRect, radius, radius);
        textRect = keyRect;
    } else {
        // Stepped key (JIS Return): the first row unit is `topWidth` units wide,
        // everything below is the key's own width, right-aligned. The two parts
        // are drawn as one united path; they overlap by a pixel so the union
        // stays a single connected shape.
        const QRectF body = QRectF(bodyRect).translated(keyRect.topLeft() - QPointF(rect.topLeft()))
                                   .adjusted(borderWidth / 2.0, borderWidth / 2.0, -borderWidth / 2.0,
                                             -borderWidth / 2.0);
        const QRectF top(keyRect.x(), keyRect.y(), keyRect.width(), body.top() - keyRect.y() + 1.0);

        QPainterPath topPath;
        topPath.addRoundedRect(top, radius, radius);
        QPainterPath bodyPath;
        bodyPath.addRoundedRect(body, radius, radius);
        painter.drawPath(topPath.united(bodyPath));
        textRect = body;
    }

    const qreal padding = qMax(2.0, theme_.padding * scale);
    textRect = textRect.adjusted(padding, padding / 2, -padding, -padding / 2);

    QFont font(theme_.fontFamily);
    font.setPixelSize(qMax(6, qRound(theme_.fontPx * scale)));
    const bool hasSmallText = !visual.sublabel.isEmpty() || !visual.kana.isEmpty();
    if (hasSmallText) {
        QFont small(theme_.fontFamily);
        small.setPixelSize(qMax(5, qRound(theme_.labelPx * scale)));
        painter.setFont(small);
        painter.setPen(color(theme_.keyText));
        if (!visual.sublabel.isEmpty())
            painter.drawText(textRect, Qt::AlignLeft | Qt::AlignTop, visual.sublabel);
        if (!visual.kana.isEmpty())
            painter.drawText(textRect, Qt::AlignRight | Qt::AlignBottom, visual.kana);
    }

    if (!visual.label.isEmpty()) {
        const QFontMetricsF metrics(font);
        const qreal available = textRect.width();
        const qreal wanted = metrics.horizontalAdvance(visual.label);
        if (wanted > available && wanted > 0.0) {
            const double factor = available / wanted;
            font.setPixelSize(qMax(6, int(font.pixelSize() * factor)));
        }
        painter.setFont(font);
        painter.setPen(color(theme_.keyText));
        QRectF labelRect = textRect;
        if (!visual.sublabel.isEmpty())
            labelRect.setTop(labelRect.top() + qMax(0.0, theme_.labelPx * scale - padding) / 2.0);
        painter.drawText(labelRect, Qt::AlignCenter, visual.label);
    }

    painter.restore();
}

} // namespace osk
