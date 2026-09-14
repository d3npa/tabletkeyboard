#include "core/geometry.h"
#include "core/layout.h"
#include "platform/x11/symresolver_x11.h"

#include <QtTest>

using namespace osk;

namespace {

QVector<const Block *> blocksOf(const LayoutSet &set, const QString &modeId, const QString &layerName)
{
    QVector<const Block *> blocks;
    const Mode *mode = set.mode(modeId);
    const Layer *layer = mode ? mode->layer(layerName) : nullptr;
    if (!layer)
        return blocks;
    for (const Block &block : layer->blocks)
        blocks.append(&block);
    return blocks;
}

const KeyPlacement *findPlacement(const LayerGeometry &geometry, quint32 keysym)
{
    for (const BlockPlacement &block : geometry.blocks) {
        for (const KeyPlacement &placement : block.keys) {
            if (placement.key && placement.key->symCode == keysym)
                return &placement;
        }
    }
    return nullptr;
}

} // namespace

class TestGeometry : public QObject
{
    Q_OBJECT
private slots:
    void jisReturnIsStepped();
    void frowAndNumpadHidden();
    void frowCentredWhenShown();
    void fitKeyUnitFitsWidth();
    void shippedBlockIds();

private:
    LayoutSet load(const QString &id) const
    {
        const QString path = QStringLiteral(TABLETKEYBOARD_DATA_DIR)
                + QStringLiteral("/layouts/") + id + QStringLiteral(".json");
        LayoutSet set;
        QString error;
        if (!LayoutSet::loadFile(path, &resolver_, &set, &error))
            qWarning("%s", qPrintable(error));
        return set;
    }

    quint32 keysym(const char *name) const
    {
        quint32 code = 0;
        if (!resolver_.fromName(QString::fromLatin1(name), &code))
            return 0;
        return code;
    }

    XlibKeysymResolver resolver_;
};

void TestGeometry::jisReturnIsStepped()
{
    const LayoutSet set = load(QStringLiteral("jp106"));
    const QVector<const Block *> blocks = blocksOf(set, QStringLiteral("full"), QStringLiteral("main"));
    QVERIFY(!blocks.isEmpty());

    const LayerGeometry geometry = computeLayerGeometry(blocks, {}, { 44.0, 2 });

    const KeyPlacement *ret = findPlacement(geometry, keysym("Return"));
    QVERIFY(ret);
    QVERIFY(!ret->bodyRect.isEmpty());
    QCOMPARE(ret->rect.width(), 66);  // topWidth 1.5 u
    QCOMPARE(ret->rect.height(), 90); // 2 rows and the gap between them
    QCOMPARE(ret->bodyRect.width(), 55); // width 1.25 u
    QCOMPARE(ret->bodyRect.x(), ret->rect.x() + 11);
    QCOMPARE(ret->bodyRect.right(), ret->rect.right());
    QCOMPARE(ret->bodyRect.bottom(), ret->rect.bottom());

    const KeyPlacement *bracket = findPlacement(geometry, keysym("bracketleft"));
    QVERIFY(bracket);
    QCOMPARE(ret->rect.x() - bracket->rect.right() - 1, 2); // exactly one gap apart
}

void TestGeometry::frowAndNumpadHidden()
{
    const LayoutSet set = load(QStringLiteral("us"));
    const QVector<const Block *> blocks = blocksOf(set, QStringLiteral("full"), QStringLiteral("main"));
    QVERIFY(!blocks.isEmpty());

    const LayerGeometry shown = computeLayerGeometry(blocks, {}, { 44.0, 2 });
    const LayerGeometry hidden = computeLayerGeometry(blocks, { QStringLiteral("frow"), QStringLiteral("numpad") },
                                                      { 44.0, 2 });

    QCOMPARE(shown.blocks.size(), 3);
    QCOMPARE(hidden.blocks.size(), 2);
    QCOMPARE(hidden.blocks.first().rect.height(), 228); // 5 rows of 44 px and 4 gaps
    QVERIFY(findPlacement(shown, keysym("F1")));
    QVERIFY(!findPlacement(hidden, keysym("F1")));
    QVERIFY(hidden.size.height() < shown.size.height());
}

void TestGeometry::frowCentredWhenShown()
{
    const LayoutSet set = load(QStringLiteral("us"));
    const QVector<const Block *> blocks = blocksOf(set, QStringLiteral("full"), QStringLiteral("main"));
    QVERIFY(!blocks.isEmpty());

    const LayerGeometry geometry = computeLayerGeometry(blocks, { QStringLiteral("numpad") }, { 44.0, 2 });
    // Widest row: 15 u in 14 keys, so 13 gaps between them.
    QCOMPARE(geometry.blocks.first().rect.width(), 15 * 44 + 13 * 2);
    QCOMPARE(geometry.blocks.first().keys.first().rect.x(), 44); // 13 u row inside a 15 u block

    // Blocks are laid out left to right; the nav block's keys follow the main block.
    const KeyPlacement *left = findPlacement(geometry, keysym("Left"));
    QVERIFY(left);
    QCOMPARE(geometry.blocks.size(), 2);
    QCOMPARE(left->rect.x(), geometry.blocks.first().rect.width() + 2);
    QCOMPARE(left->rect.y(), 44 + 3 * (44 + 2)); // nav topGap + three rows
}

void TestGeometry::fitKeyUnitFitsWidth()
{
    const LayoutSet set = load(QStringLiteral("us"));
    const QVector<const Block *> blocks = blocksOf(set, QStringLiteral("full"), QStringLiteral("main"));
    QVERIFY(!blocks.isEmpty());

    const double gapRatio = 2.0 / 72.0;
    const double paddingRatio = 4.0 / 72.0;
    const double fit = fitKeyUnit(blocks, { QStringLiteral("frow"), QStringLiteral("numpad") }, gapRatio,
                                  paddingRatio, 1000);
    const double expected = 1000.0 / (18.0 + gapRatio + 2.0 * paddingRatio);
    QVERIFY(qAbs(fit - expected) < 0.5);
}

void TestGeometry::shippedBlockIds()
{
    for (const QString &id : { QStringLiteral("us"), QStringLiteral("jp106") }) {
        const LayoutSet set = load(id);
        const QVector<const Block *> blocks = blocksOf(set, QStringLiteral("full"), QStringLiteral("main"));
        QVERIFY(!blocks.isEmpty());

        QSet<QString> blockIds;
        QSet<QString> rowIds;
        for (const Block *block : blocks) {
            blockIds.insert(block->id);
            for (const KeyRow &row : block->rows)
                rowIds.insert(row.id);
        }
        QVERIFY(blockIds.contains(QStringLiteral("nav")));
        QVERIFY(blockIds.contains(QStringLiteral("numpad")));
        QVERIFY(rowIds.contains(QStringLiteral("frow")));
    }
}

QTEST_GUILESS_MAIN(TestGeometry)
#include "tst_geometry.moc"
