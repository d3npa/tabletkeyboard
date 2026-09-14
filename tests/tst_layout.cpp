#include "core/layout.h"
#include "core/lint.h"
#include "platform/x11/symresolver_x11.h"

#include <QDir>
#include <QTemporaryDir>
#include <QtTest>

using namespace osk;

namespace {

const KeyDef *findKey(const LayoutSet &set, const QString &modeId, const QString &layerName,
                      const QString &symName, const KeysymResolver *resolver)
{
    const Mode *mode = set.mode(modeId);
    const Layer *layer = mode ? mode->layer(layerName) : nullptr;
    if (!layer)
        return nullptr;
    quint32 code = 0;
    if (!resolver->fromName(symName, &code))
        return nullptr;
    for (const Block &block : layer->blocks) {
        for (const KeyRow &row : block.rows) {
            for (const KeyDef &key : row.keys) {
                if (key.type == KeyDef::Key && key.symCode == code)
                    return &key;
            }
        }
    }
    return nullptr;
}

// First key of the layer whose label matches, whatever its type.
const KeyDef *findKeyByLabel(const LayoutSet &set, const QString &modeId, const QString &layerName,
                             const QString &label)
{
    const Mode *mode = set.mode(modeId);
    const Layer *layer = mode ? mode->layer(layerName) : nullptr;
    if (!layer)
        return nullptr;
    for (const Block &block : layer->blocks) {
        for (const KeyRow &row : block.rows) {
            for (const KeyDef &key : row.keys) {
                if (key.label == label)
                    return &key;
            }
        }
    }
    return nullptr;
}

// label -> printed kana for every key that carries one.
QHash<QString, QString> kanaByLabel(const LayoutSet &set, const QString &modeId, const QString &layerName)
{
    QHash<QString, QString> kana;
    const Mode *mode = set.mode(modeId);
    const Layer *layer = mode ? mode->layer(layerName) : nullptr;
    if (!layer)
        return kana;
    for (const Block &block : layer->blocks) {
        for (const KeyRow &row : block.rows) {
            for (const KeyDef &key : row.keys) {
                if (!key.kana.isEmpty())
                    kana.insert(key.label, key.kana);
            }
        }
    }
    return kana;
}

} // namespace

class TestLayout : public QObject
{
    Q_OBJECT
private slots:
    void loadsShippedLayouts();
    void resolvesEveryKeysym();
    void parsesSteppedKeysAndFn();
    void jp106KanaLegends();
    void jp106ZeroKeyHidesShiftHint();
    void navClusterHasPrintKeys();
    void rejectsMalformedJson();
    void rejectsUnknownKeysym();
    void rejectsUnknownFnKeysym();
    void parsesRowObjectsAndArrayRows();
    void rejectsUnknownAction();
    void rejectsThemeFile();
    void lintShippedLayouts();

private:
    QString dataPath(const QString &file) const
    {
        return QStringLiteral(TABLETKEYBOARD_DATA_DIR) + QStringLiteral("/layouts/") + file;
    }

    bool load(const QString &file, LayoutSet *out, QString *error) const
    {
        XlibKeysymResolver resolver;
        return LayoutSet::loadFile(dataPath(file), &resolver, out, error);
    }
};

void TestLayout::loadsShippedLayouts()
{
    for (const QString &id : { QStringLiteral("us"), QStringLiteral("jp106") }) {
        LayoutSet set;
        QString error;
        QVERIFY2(load(id + QStringLiteral(".json"), &set, &error), qPrintable(error));
        QCOMPARE(set.id, id);
        QVERIFY(set.mode(QStringLiteral("full")));
        QVERIFY(set.mode(QStringLiteral("simple")));
        for (const Mode &mode : set.modes) {
            QVERIFY(mode.primaryLayer());
            QVERIFY(!mode.primaryLayer()->blocks.isEmpty());
        }
    }
}

void TestLayout::resolvesEveryKeysym()
{
    XlibKeysymResolver resolver;
    for (const QString &id : { QStringLiteral("us"), QStringLiteral("jp106") }) {
        LayoutSet set;
        QString error;
        QVERIFY2(LayoutSet::loadFile(dataPath(id + QStringLiteral(".json")), &resolver, &set, &error),
                 qPrintable(error));

        int keys = 0;
        for (const Mode &mode : set.modes) {
            for (const Layer &layer : mode.layers) {
                for (const Block &block : layer.blocks) {
                    for (const KeyRow &row : block.rows) {
                        for (const KeyDef &key : row.keys) {
                            if (key.type != KeyDef::Key)
                                continue;
                            ++keys;
                            QVERIFY2(key.symCode != 0, qPrintable(key.sym));
                            if (!key.shifted.isEmpty()) {
                                QVERIFY2(key.shiftedCode != 0, qPrintable(key.shifted));
                                QVERIFY(key.shiftedCode != key.symCode);
                            }
                            if (!key.fn.isEmpty()) {
                                QVERIFY2(key.fnCode != 0, qPrintable(key.fn));
                                QVERIFY(key.fnCode != key.symCode);
                            }
                        }
                    }
                }
            }
        }
        QVERIFY(keys > 40);
    }
}

void TestLayout::parsesSteppedKeysAndFn()
{
    XlibKeysymResolver resolver;

    LayoutSet jp;
    QString error;
    QVERIFY2(load(QStringLiteral("jp106.json"), &jp, &error), qPrintable(error));
    const KeyDef *ret = findKey(jp, QStringLiteral("full"), QStringLiteral("main"),
                                QStringLiteral("Return"), &resolver);
    QVERIFY(ret);
    QCOMPARE(ret->width, 1.25);
    QCOMPARE(ret->topWidth, 1.5);
    QCOMPARE(ret->height, 2.0);

    LayoutSet us;
    QVERIFY2(load(QStringLiteral("us.json"), &us, &error), qPrintable(error));
    const KeyDef *one = findKey(us, QStringLiteral("full"), QStringLiteral("main"), QStringLiteral("1"),
                                &resolver);
    QVERIFY(one);
    QCOMPARE(one->fn, QStringLiteral("F1"));
    QVERIFY(one->fnCode != 0);
    QCOMPARE(one->fnLabel, QStringLiteral("F1")); // no fnLabel in the file: the keysym name is used
}

void TestLayout::jp106KanaLegends()
{
    LayoutSet jp;
    QString error;
    QVERIFY2(load(QStringLiteral("jp106.json"), &jp, &error), qPrintable(error));

    const QHash<QString, QString> kana = kanaByLabel(jp, QStringLiteral("full"), QStringLiteral("main"));
    const QHash<QString, QString> expected = {
        { QStringLiteral("1"), QStringLiteral("ぬ") },
        { QStringLiteral("0"), QStringLiteral("わ") },
        { QStringLiteral("-"), QStringLiteral("ほ") },
        { QStringLiteral("^"), QStringLiteral("へ") },
        { QStringLiteral("¥"), QStringLiteral("ー") },
        { QStringLiteral("Q"), QStringLiteral("た") },
        { QStringLiteral("@"), QStringLiteral("゛") }, // voicedsound, AD11
        { QStringLiteral("["), QStringLiteral("゜") }, // semivoicedsound, AD12
        { QStringLiteral("A"), QStringLiteral("ち") },
        { QStringLiteral(";"), QStringLiteral("れ") },
        { QStringLiteral("]"), QStringLiteral("む") },
        { QStringLiteral("Z"), QStringLiteral("つ") },
        { QStringLiteral(","), QStringLiteral("ね") },
        { QStringLiteral("/"), QStringLiteral("め") },
        { QStringLiteral("\\"), QStringLiteral("ろ") },
    };
    for (auto it = expected.constBegin(); it != expected.constEnd(); ++it)
        QCOMPARE(kana.value(it.key()), it.value());

    // Keys that only exist on a Latin or IME legend stay bare.
    for (const QString &label : { QStringLiteral("半/全"), QStringLiteral("Tab"), QStringLiteral("Shift"),
                                  QStringLiteral("Ins"), QStringLiteral("変換") }) {
        const KeyDef *key = findKeyByLabel(jp, QStringLiteral("full"), QStringLiteral("main"), label);
        QVERIFY2(key, qPrintable(label));
        QVERIFY2(key->kana.isEmpty(), qPrintable(label));
    }

    // The thumb layout is a different arrangement with no photo reference.
    QVERIFY(kanaByLabel(jp, QStringLiteral("simple"), QStringLiteral("main")).isEmpty());
}

void TestLayout::jp106ZeroKeyHidesShiftHint()
{
    LayoutSet jp;
    QString error;
    QVERIFY2(load(QStringLiteral("jp106.json"), &jp, &error), qPrintable(error));

    // JIS keycaps print no shifted legend on the 0 key, but the key still sends
    // the shifted keysym: the painted hint is blanked, not the binding.
    const KeyDef *zero = findKeyByLabel(jp, QStringLiteral("full"), QStringLiteral("main"),
                                        QStringLiteral("0"));
    QVERIFY(zero);
    QVERIFY(zero->shiftLabelSet);
    QVERIFY(zero->shiftLabel.isEmpty());
    QVERIFY(zero->shiftedCode != 0);
}

void TestLayout::navClusterHasPrintKeys()
{
    XlibKeysymResolver resolver;
    for (const QString &id : { QStringLiteral("us"), QStringLiteral("jp106") }) {
        LayoutSet set;
        QString error;
        QVERIFY2(load(id + QStringLiteral(".json"), &set, &error), qPrintable(error));

        const Mode *mode = set.mode(QStringLiteral("full"));
        QVERIFY(mode);
        const Layer *layer = mode->primaryLayer();
        QVERIFY(layer);

        const Block *nav = nullptr;
        const Block *numpad = nullptr;
        for (const Block &block : layer->blocks) {
            if (block.id == QLatin1String("nav"))
                nav = &block;
            else if (block.id == QLatin1String("numpad"))
                numpad = &block;
        }
        QVERIFY(nav);
        QVERIFY(numpad);
        QVERIFY(!nav->rows.isEmpty());
        QVERIFY(!numpad->rows.isEmpty());
        QCOMPARE(nav->rows.first().id, QStringLiteral("frowgap"));
        QCOMPARE(numpad->rows.first().id, QStringLiteral("frowgap"));

        // Print/Scroll/Pause sit directly under the gutter row.
        QVERIFY(nav->rows.size() > 1);
        const KeyRow &row = nav->rows.at(1);
        QCOMPARE(row.keys.size(), 3);

        quint32 print = 0, sysReq = 0, scroll = 0, pause = 0, brk = 0;
        QVERIFY(resolver.fromName(QStringLiteral("Print"), &print));
        QVERIFY(resolver.fromName(QStringLiteral("Sys_Req"), &sysReq));
        QVERIFY(resolver.fromName(QStringLiteral("Scroll_Lock"), &scroll));
        QVERIFY(resolver.fromName(QStringLiteral("Pause"), &pause));
        QVERIFY(resolver.fromName(QStringLiteral("Break"), &brk));

        QCOMPARE(row.keys.at(0).symCode, print);
        QCOMPARE(row.keys.at(0).shiftedCode, sysReq);
        QCOMPARE(row.keys.at(1).symCode, scroll);
        QCOMPARE(row.keys.at(1).indicator, QStringLiteral("scroll"));
        QVERIFY(!row.keys.at(1).repeat);
        QCOMPARE(row.keys.at(2).symCode, pause);
        QCOMPARE(row.keys.at(2).shiftedCode, brk);
    }
}

void TestLayout::rejectsMalformedJson()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("us.json"));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("{ \"id\": \"us\", ");
    file.close();

    XlibKeysymResolver resolver;
    LayoutSet set;
    QString error;
    QVERIFY(!LayoutSet::loadFile(path, &resolver, &set, &error));
    QVERIFY(!error.isEmpty());
}

void TestLayout::rejectsUnknownKeysym()
{
    QTemporaryDir dir;
    const QString path = dir.filePath(QStringLiteral("bogus.json"));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(R"({"id":"bogus","name":"Bogus","modes":{"full":{"layers":[
        {"name":"main","blocks":[{"rows":[[{"label":"?","sym":"NotAKeysym"}]]}]}]}}})");
    file.close();

    XlibKeysymResolver resolver;
    LayoutSet set;
    QString error;
    QVERIFY(!LayoutSet::loadFile(path, &resolver, &set, &error));
    QVERIFY(error.contains(QStringLiteral("NotAKeysym")));
}

void TestLayout::rejectsUnknownFnKeysym()
{
    QTemporaryDir dir;
    const QString path = dir.filePath(QStringLiteral("bogus.json"));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(R"({"id":"bogus","name":"Bogus","modes":{"full":{"layers":[
        {"name":"main","blocks":[{"rows":[[{"label":"1","sym":"1","fn":"NotAKeysym"}]]}]}]}}})");
    file.close();

    XlibKeysymResolver resolver;
    LayoutSet set;
    QString error;
    QVERIFY(!LayoutSet::loadFile(path, &resolver, &set, &error));
    QVERIFY(error.contains(QStringLiteral("NotAKeysym")));
}

void TestLayout::parsesRowObjectsAndArrayRows()
{
    QTemporaryDir dir;
    const QString path = dir.filePath(QStringLiteral("rows.json"));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(R"({"id":"rows","name":"Rows","modes":{"full":{"layers":[
        {"name":"main","blocks":[{"id":"blk","rows":[
            {"id":"frow","keys":[{"label":"Esc","sym":"Escape"}]},
            [{"label":"a","sym":"a","shifted":"A"}]]}]}]}}})");
    file.close();

    XlibKeysymResolver resolver;
    LayoutSet set;
    QString error;
    QVERIFY2(LayoutSet::loadFile(path, &resolver, &set, &error), qPrintable(error));
    const Mode *mode = set.mode(QStringLiteral("full"));
    QVERIFY(mode);
    const Layer *layer = mode->primaryLayer();
    QVERIFY(layer && !layer->blocks.isEmpty());
    const Block &block = layer->blocks.first();
    QCOMPARE(block.id, QStringLiteral("blk"));
    QCOMPARE(block.rows.size(), 2);
    QCOMPARE(block.rows.at(0).id, QStringLiteral("frow"));
    QCOMPARE(block.rows.at(0).keys.size(), 1);
    QVERIFY(block.rows.at(1).id.isEmpty());
    QCOMPARE(block.rows.at(1).keys.size(), 1);
    QVERIFY(block.rows.at(1).keys.first().shiftedCode != 0);
}

void TestLayout::rejectsUnknownAction()
{
    QTemporaryDir dir;
    const QString path = dir.filePath(QStringLiteral("bogus.json"));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(R"({"id":"bogus","name":"Bogus","modes":{"full":{"layers":[
        {"name":"main","blocks":[{"rows":[[{"type":"action","action":"explode"}]]}]}]}}})");
    file.close();

    XlibKeysymResolver resolver;
    LayoutSet set;
    QString error;
    QVERIFY(!LayoutSet::loadFile(path, &resolver, &set, &error));
    QVERIFY(error.contains(QStringLiteral("explode")));
}

void TestLayout::rejectsThemeFile()
{
    const QString path = QStringLiteral(TABLETKEYBOARD_DATA_DIR) + QStringLiteral("/themes/default.json");
    XlibKeysymResolver resolver;
    LayoutSet set;
    QString error;
    QVERIFY(!LayoutSet::loadFile(path, &resolver, &set, &error));
    QVERIFY2(error.contains(QStringLiteral("looks like a theme")), qPrintable(error));
}

void TestLayout::lintShippedLayouts()
{
    XlibKeysymResolver resolver;
    for (const QString &id : { QStringLiteral("us"), QStringLiteral("jp106") }) {
        const QString path = dataPath(id + QStringLiteral(".json"));
        const QVector<LintIssue> issues = Lint::checkLayoutFile(path, &resolver);
        for (const LintIssue &issue : issues)
            qInfo("%s", qPrintable(issue.toString()));
        QVERIFY(!Lint::hasErrors(issues));
    }
}

QTEST_GUILESS_MAIN(TestLayout)
#include "tst_layout.moc"
