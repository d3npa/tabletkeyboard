// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 d3npa <gh@w1t.ch>
#include "app/icon.h"

#include <QPainter>
#include <QPixmap>

namespace osk {

QIcon appIcon()
{
    QIcon icon;
    for (const int size : { 16, 22, 32, 48, 64 }) {
        QPixmap pixmap(size, size);
        pixmap.fill(Qt::transparent);

        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing, true);
        const double s = size / 64.0;

        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(0x2b, 0x2b, 0x2b));
        painter.drawRoundedRect(QRectF(2 * s, 14 * s, 60 * s, 36 * s), 6 * s, 6 * s);

        painter.setBrush(QColor(0xf2, 0xf2, 0xf2));
        const double keyW = 7 * s;
        const double keyH = 6 * s;
        const double stepX = 8.4 * s;
        for (int row = 0; row < 3; ++row) {
            for (int col = 0; col < 6; ++col) {
                if (row == 2 && (col == 2 || col == 3))
                    continue; // the spacebar spans these two
                const double w = (row == 1 && col == 5) ? 13 * s : keyW;
                painter.drawRoundedRect(QRectF(7.5 * s + col * stepX, (18 + row * 9) * s, w, keyH),
                                        1.5 * s, 1.5 * s);
            }
        }
        painter.drawRoundedRect(QRectF(7.5 * s + 2 * stepX, 36 * s, 2 * keyW + 1.4 * s, keyH), 1.5 * s, 1.5 * s);

        icon.addPixmap(pixmap);
    }
    return icon;
}

} // namespace osk
