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

} // namespace

class TestLayout : public QObject
{
    Q_OBJECT
private slots:
    void loadsShippedLayouts();
    void resolvesEveryKeysym();
    void parsesSteppedKeysAndFn();
    void rejectsMalformedJson();
    void rejectsUnknownKeysym();
    void rejectsUnknownFnKeysym();
    void parsesRowObjectsAndArrayRows();
    void rejectsUnknownAction();
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
