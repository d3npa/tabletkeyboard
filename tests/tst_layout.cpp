#include "core/layout.h"
#include "core/lint.h"
#include "platform/x11/symresolver_x11.h"

#include <QDir>
#include <QTemporaryDir>
#include <QtTest>

using namespace osk;

class TestLayout : public QObject
{
    Q_OBJECT
private slots:
    void loadsShippedLayouts();
    void resolvesEveryKeysym();
    void rejectsMalformedJson();
    void rejectsUnknownKeysym();
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
                        for (const KeyDef &key : row) {
                            if (key.type != KeyDef::Key)
                                continue;
                            ++keys;
                            QVERIFY2(key.symCode != 0, qPrintable(key.sym));
                            if (!key.shifted.isEmpty()) {
                                QVERIFY2(key.shiftedCode != 0, qPrintable(key.shifted));
                                QVERIFY(key.shiftedCode != key.symCode);
                            }
                        }
                    }
                }
            }
        }
        QVERIFY(keys > 40);
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
