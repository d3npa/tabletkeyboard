#pragma once

#include "core/layout.h"

#include <QRect>
#include <QSet>
#include <QSize>
#include <QString>
#include <QVector>

namespace osk {

struct GeometryMetrics
{
    double unit = 44.0; // px per key unit
    int gap = 2;        // px between keys, rows and blocks
};

struct KeyPlacement
{
    const KeyDef *key = nullptr;
    QRect rect;     // widget rect, content coordinates
    QRect bodyRect; // stepped keys: part below the first row unit, same space as rect
};

struct BlockPlacement
{
    QRect rect;
    QVector<KeyPlacement> keys;
};

struct LayerGeometry
{
    QSize size; // content size
    QVector<BlockPlacement> blocks;
};

// Lays out one layer in absolute pixels: blocks left to right, rows centred
// inside their block, keys advanced by their width. `hiddenIds` skips blocks
// and rows with that id; rows keep their order.
LayerGeometry computeLayerGeometry(const QVector<const Block *> &blocks, const QSet<QString> &hiddenIds,
                                   const GeometryMetrics &metrics);

// Largest key unit whose content (gaps and padding included, both scaled with
// the unit) still fits availableWidth.
double fitKeyUnit(const QVector<const Block *> &blocks, const QSet<QString> &hiddenIds,
                  double gapRatio, double paddingRatio, int availableWidth);

} // namespace osk
