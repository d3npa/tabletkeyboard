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
    void variantListing();
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
    for (const QString &id : { QStringLiteral("default"), QStringLiteral("default-dark"),
                               QStringLiteral("gold-light"), QStringLiteral("gold-dark") }) {
        ThemeSpec theme;
        QString error;
        QVERIFY2(ThemeSpec::loadFile(themePath(id + QStringLiteral(".json")), &theme, &error),
                 qPrintable(error));
        QCOMPARE(theme.id, id);
        QVERIFY(!theme.name.isEmpty());
        QCOMPARE(theme.dark, id == QStringLiteral("gold-dark") || id == QStringLiteral("default-dark"));
        QVERIFY(isValidColor(theme.keyTop));
        QVERIFY(isValidColor(theme.windowBg));
        if (id.startsWith(QStringLiteral("gold")))
            QVERIFY2(isValidColor(theme.keyMid), qPrintable(id));
        else
            QVERIFY(theme.keyMid.isEmpty());
        QVERIFY(theme.keyUnit >= 8);
        QVERIFY(theme.fontPx >= 6);
        QVERIFY(theme.barHeight >= 8);
        QVERIFY(theme.windowOpacity > 0.0 && theme.windowOpacity <= 1.0);
        QVERIFY(!Lint::hasErrors(Lint::check(theme)));
    }
}

void TestTheme::variantListing()
{
    ThemeLibrary library;
    library.scan();
    QStringList lightIds;
    QStringList darkIds;
    for (const ThemeSpec *theme : library.byVariant(false)) {
        QVERIFY(!theme->dark);
        lightIds << theme->id;
    }
    for (const ThemeSpec *theme : library.byVariant(true)) {
        QVERIFY(theme->dark);
        darkIds << theme->id;
    }
    QVERIFY(lightIds.contains(QStringLiteral("gold-light")));
    QVERIFY(lightIds.contains(QStringLiteral("default")));
    QVERIFY(darkIds.contains(QStringLiteral("gold-dark")));
    QVERIFY(darkIds.contains(QStringLiteral("default-dark")));
    QVERIFY(!lightIds.contains(QStringLiteral("default-dark")));
    QVERIFY(!darkIds.contains(QStringLiteral("gold-light")));
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
