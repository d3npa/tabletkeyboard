#include "core/geometry.h"

namespace osk {

namespace {

bool isHidden(const QString &id, const QSet<QString> &hiddenIds)
{
    return !id.isEmpty() && hiddenIds.contains(id);
}

// Key units of the widest row that is not hidden; zero when every row is.
double visibleBlockUnits(const Block &block, const QSet<QString> &hiddenIds)
{
    double units = 0;
    for (const KeyRow &row : block.rows) {
        if (isHidden(row.id, hiddenIds) || row.keys.isEmpty())
            continue;
        units = qMax(units, row.widthUnits());
    }
    return units;
}

// Pixels one row occupies, gaps between its keys included.
int rowPixelWidth(const KeyRow &row, const GeometryMetrics &metrics)
{
    int width = 0;
    for (const KeyDef &key : row.keys)
        width += qRound(qMax(key.width, key.topWidth) * metrics.unit) + metrics.gap;
    if (!row.keys.isEmpty())
        width -= metrics.gap;
    return width;
}

} // namespace

LayerGeometry computeLayerGeometry(const QVector<const Block *> &blocks, const QSet<QString> &hiddenIds,
                                   const GeometryMetrics &metrics)
{
    LayerGeometry geometry;

    const int pitch = qRound(metrics.unit) + metrics.gap;
    int x = 0;
    int contentHeight = 0;

    for (const Block *block : blocks) {
        if (!block || isHidden(block->id, hiddenIds))
            continue;

        const double blockUnits = visibleBlockUnits(*block, hiddenIds);
        if (blockUnits <= 0)
            continue;

        BlockPlacement placement;
        int y = qRound(block->topGap * qRound(metrics.unit));
        int blockWidth = 0;
        int blockBottom = 0;

        for (const KeyRow &row : block->rows) {
            if (isHidden(row.id, hiddenIds) || row.keys.isEmpty())
                continue;

            const int lead = qRound((blockUnits - row.widthUnits()) * metrics.unit / 2.0);
            int keyX = lead;
            for (const KeyDef &key : row.keys) {
                KeyPlacement keyPlacement;
                keyPlacement.key = &key;

                const int width = qRound(qMax(key.width, key.topWidth) * metrics.unit);
                const int height = qRound(key.height * metrics.unit + (key.height - 1.0) * metrics.gap);
                keyPlacement.rect = QRect(x + keyX, y, width, height);

                if (key.topWidth > key.width) {
                    const int bodyWidth = qRound(key.width * metrics.unit);
                    const int topHeight = qRound(metrics.unit) + metrics.gap;
                    keyPlacement.bodyRect = QRect(x + keyX + (width - bodyWidth), y + topHeight, bodyWidth,
                                                  height - topHeight);
                }

                placement.keys.append(keyPlacement);
                blockBottom = qMax(blockBottom, keyPlacement.rect.bottom() + 1);
                keyX += width + metrics.gap;
            }
            blockWidth = qMax(blockWidth, lead + rowPixelWidth(row, metrics));
            y += pitch;
            blockBottom = qMax(blockBottom, y - metrics.gap);
        }

        placement.rect = QRect(x, 0, blockWidth, blockBottom);
        geometry.blocks.append(placement);

        x += blockWidth + metrics.gap;
        contentHeight = qMax(contentHeight, blockBottom);
    }

    geometry.size = QSize(x > 0 ? x - metrics.gap : 0, contentHeight);
    return geometry;
}

double fitKeyUnit(const QVector<const Block *> &blocks, const QSet<QString> &hiddenIds,
                  double gapRatio, double paddingRatio, int availableWidth)
{
    double units = 0;
    int visibleBlocks = 0;
    for (const Block *block : blocks) {
        if (!block || isHidden(block->id, hiddenIds))
            continue;
        const double blockUnits = visibleBlockUnits(*block, hiddenIds);
        if (blockUnits <= 0)
            continue;
        units += blockUnits;
        ++visibleBlocks;
    }
    if (units <= 0)
        return 0;

    const double denominator = units + gapRatio * qMax(0, visibleBlocks - 1) + 2.0 * paddingRatio;
    if (denominator <= 0)
        return 0;
    return availableWidth / denominator;
}

} // namespace osk
