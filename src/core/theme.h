#pragma once

#include <QJsonObject>
#include <QString>
#include <QStringList>
#include <QVector>

namespace osk {

// A theme is pure data; colours stay strings so that osk-core does not need
// QtGui. The UI layer converts them to QColor.
struct ThemeSpec
{
    QString id;
    QString name;

    QString keyTop = QStringLiteral("#fdfdfd");
    QString keyBottom = QStringLiteral("#f0f0f0");
    QString keyBorder = QStringLiteral("#c8c8c8");
    QString keyText = QStringLiteral("#1a1a1a");
    QString keyPressedTop = QStringLiteral("#dcdcdc");
    QString keyPressedBottom = QStringLiteral("#c8c8c8");
    QString keyHover = QStringLiteral("#ecf6ff");
    QString modActive = QStringLiteral("#cce4f7");
    QString accent = QStringLiteral("#0078d7");
    QString barBg = QStringLiteral("#f4f4f4");
    QString windowBg = QStringLiteral("#e8e8e8");
    double windowOpacity = 1.0;

    int radius = 3;
    int border = 1;
    int gap = 2;
    int padding = 4;
    int barHeight = 26;
    int fontPx = 15;
    int labelPx = 11;
    int keyUnit = 44;
    QString fontFamily = QStringLiteral("Noto Sans");

    static bool fromJson(const QJsonObject &obj, ThemeSpec *out, QString *error);
    static bool loadFile(const QString &path, ThemeSpec *out, QString *error);
};

class ThemeLibrary
{
public:
    ThemeLibrary() = default;

    void scan();

    const QVector<ThemeSpec> &themes() const { return themes_; }
    const ThemeSpec *byId(const QString &id) const;
    const QStringList &errors() const { return errors_; }

private:
    void loadResourceDir(const QString &path);
    void loadDir(const QString &path);
    void insertOrReplace(ThemeSpec &&theme);

    QVector<ThemeSpec> themes_;
    QStringList errors_;
};

// True when the colour parses as #rrggbb or #rrggbbaa.
bool isValidColor(const QString &value);

} // namespace osk
