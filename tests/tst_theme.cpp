#include "core/lint.h"
#include "core/theme.h"

#include <QTemporaryDir>
#include <QtTest>

using namespace osk;

class TestTheme : public QObject
{
    Q_OBJECT
private slots:
    void loadsShippedThemes();
    void rejectsMalformedColor();
    void rejectsBadOpacity();
    void rejectsMissingId();
    void missingMetricsUseDefaults();
};

QString themePath(const QString &file)
{
    return QStringLiteral(TABLETKEYBOARD_DATA_DIR) + QStringLiteral("/themes/") + file;
}

void TestTheme::loadsShippedThemes()
{
    for (const QString &id : { QStringLiteral("win10"), QStringLiteral("win10-dark"),
                               QStringLiteral("minimal"), QStringLiteral("letsnote-gold") }) {
        ThemeSpec theme;
        QString error;
        QVERIFY2(ThemeSpec::loadFile(themePath(id + QStringLiteral(".json")), &theme, &error),
                 qPrintable(error));
        QCOMPARE(theme.id, id);
        QVERIFY(!theme.name.isEmpty());
        QVERIFY(isValidColor(theme.keyTop));
        QVERIFY(isValidColor(theme.windowBg));
        QVERIFY(theme.keyUnit >= 8);
        QVERIFY(theme.fontPx >= 6);
        QVERIFY(theme.barHeight >= 8);
        QVERIFY(theme.windowOpacity > 0.0 && theme.windowOpacity <= 1.0);
        QVERIFY(!Lint::hasErrors(Lint::check(theme)));
    }
}

void TestTheme::rejectsMalformedColor()
{
    QTemporaryDir dir;
    const QString path = dir.filePath(QStringLiteral("bad.json"));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(R"({"id":"bad","name":"Bad","colors":{"key_top":"#zzzzzz"}})");
    file.close();

    ThemeSpec theme;
    QString error;
    QVERIFY(!ThemeSpec::loadFile(path, &theme, &error));
    QVERIFY(error.contains(QStringLiteral("key_top")));
}

void TestTheme::rejectsBadOpacity()
{
    QTemporaryDir dir;
    const QString path = dir.filePath(QStringLiteral("bad.json"));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(R"({"id":"bad","name":"Bad","colors":{"window_opacity":1.5}})");
    file.close();

    ThemeSpec theme;
    QString error;
    QVERIFY(!ThemeSpec::loadFile(path, &theme, &error));
    QVERIFY(error.contains(QStringLiteral("window_opacity")));
}

void TestTheme::rejectsMissingId()
{
    QTemporaryDir dir;
    const QString path = dir.filePath(QStringLiteral("bad.json"));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(R"({"name":"No id"})");
    file.close();

    ThemeSpec theme;
    QString error;
    QVERIFY(!ThemeSpec::loadFile(path, &theme, &error));
    QVERIFY(error.contains(QStringLiteral("id")));
}

void TestTheme::missingMetricsUseDefaults()
{
    QTemporaryDir dir;
    const QString path = dir.filePath(QStringLiteral("small.json"));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(R"({"id":"small","name":"Small","metrics":{"key_unit":50,"font_px":18}})");
    file.close();

    ThemeSpec theme;
    QString error;
    QVERIFY2(ThemeSpec::loadFile(path, &theme, &error), qPrintable(error));
    QCOMPARE(theme.keyUnit, 50);
    QCOMPARE(theme.fontPx, 18);
    QCOMPARE(theme.labelPx, 11); // default
    QCOMPARE(theme.barHeight, 26); // default
}

QTEST_GUILESS_MAIN(TestTheme)
#include "tst_theme.moc"
