#include "core/theme.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QRegularExpression>
#include <QStandardPaths>

namespace osk {

namespace {

bool fail(QString *error, const QString &message)
{
    if (error)
        *error = message;
    return false;
}

bool readColor(const QJsonObject &obj, const char *name, QString *target, QString *error)
{
    const QJsonValue value = obj.value(QLatin1String(name));
    if (value.isUndefined())
        return true;
    const QString color = value.toString();
    if (!isValidColor(color))
        return fail(error, QStringLiteral("color \"%1\" must be #rrggbb").arg(QLatin1String(name)));
    *target = color;
    return true;
}

bool readMetric(const QJsonObject &obj, const char *name, int *target, QString *error, int minValue)
{
    const QJsonValue value = obj.value(QLatin1String(name));
    if (value.isUndefined())
        return true;
    const double number = value.toDouble(-1);
    const int integer = qRound(number);
    if (!value.isDouble() || integer < minValue)
        return fail(error, QStringLiteral("metric \"%1\" must be a number >= %2").arg(QLatin1String(name)).arg(minValue));
    *target = integer;
    return true;
}

} // namespace

bool isValidColor(const QString &value)
{
    static const QRegularExpression re(QStringLiteral("^#[0-9a-fA-F]{6}([0-9a-fA-F]{2})?$"));
    return re.match(value).hasMatch();
}

bool ThemeSpec::fromJson(const QJsonObject &obj, ThemeSpec *out, QString *error)
{
    // A layout handed to the theme loader would otherwise load with all the
    // default colours and lint clean.
    if (obj.contains(QStringLiteral("modes")))
        return fail(error, QStringLiteral("theme: not a theme file (looks like a layout)"));

    ThemeSpec theme;
    theme.id = obj.value(QStringLiteral("id")).toString();
    if (theme.id.isEmpty())
        return fail(error, QStringLiteral("theme: missing \"id\""));
    theme.name = obj.value(QStringLiteral("name")).toString(theme.id);
    theme.dark = obj.value(QStringLiteral("dark")).toBool(false);

    const QJsonObject colors = obj.value(QStringLiteral("colors")).toObject();
    for (const ThemeColorField &field : themeColorFields) {
        if (!readColor(colors, field.name, &(theme.*field.member), error))
            return false;
    }

    if (colors.contains(QStringLiteral("window_opacity"))) {
        const double opacity = colors.value(QStringLiteral("window_opacity")).toDouble(-1);
        if (!(opacity >= 0.0 && opacity <= 1.0))
            return fail(error, QStringLiteral("colors \"window_opacity\" must be within [0, 1]"));
        theme.windowOpacity = opacity;
    }

    const QJsonObject metrics = obj.value(QStringLiteral("metrics")).toObject();
    for (const ThemeMetricField &field : themeMetricFields) {
        if (!readMetric(metrics, field.name, &(theme.*field.member), error, field.minValue))
            return false;
    }

    if (metrics.contains(QStringLiteral("font_family"))) {
        const QString family = metrics.value(QStringLiteral("font_family")).toString();
        if (family.isEmpty())
            return fail(error, QStringLiteral("metrics \"font_family\" must be a non-empty string"));
        theme.fontFamily = family;
    }

    *out = theme;
    return true;
}

bool ThemeSpec::loadFile(const QString &path, ThemeSpec *out, QString *error)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return fail(error, QStringLiteral("%1: %2").arg(path, file.errorString()));

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError)
        return fail(error, QStringLiteral("%1: %2").arg(path, parseError.errorString()));
    if (!doc.isObject())
        return fail(error, QStringLiteral("%1: top level must be an object").arg(path));

    if (!fromJson(doc.object(), out, error))
        return fail(error, QStringLiteral("%1: %2").arg(path, error ? *error : QString()));

    if (out->id != QFileInfo(path).completeBaseName())
        return fail(error, QStringLiteral("%1: id \"%2\" must match the file name").arg(path, out->id));

    return true;
}

void ThemeLibrary::scan()
{
    themes_.clear();
    errors_.clear();

    loadResourceDir(QStringLiteral(":/themes"));
    loadDir(QStringLiteral("/usr/local/share/tabletkeyboard/themes"));
    loadDir(QStringLiteral("/usr/share/tabletkeyboard/themes"));
    loadDir(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
            + QStringLiteral("/tabletkeyboard/themes"));
}

void ThemeLibrary::loadResourceDir(const QString &path)
{
    const QDir dir(path);
    const QStringList files = dir.entryList({ QStringLiteral("*.json") }, QDir::Files, QDir::Name);
    for (const QString &file : files) {
        ThemeSpec theme;
        QString error;
        if (!ThemeSpec::loadFile(dir.filePath(file), &theme, &error))
            errors_.append(error);
        else
            insertOrReplace(std::move(theme));
    }
}

void ThemeLibrary::loadDir(const QString &path)
{
    const QDir dir(path);
    if (!dir.exists())
        return;
    const QStringList files = dir.entryList({ QStringLiteral("*.json") }, QDir::Files, QDir::Name);
    for (const QString &file : files) {
        ThemeSpec theme;
        QString error;
        if (!ThemeSpec::loadFile(dir.filePath(file), &theme, &error))
            errors_.append(error);
        else
            insertOrReplace(std::move(theme));
    }
}

void ThemeLibrary::insertOrReplace(ThemeSpec &&theme)
{
    for (ThemeSpec &existing : themes_) {
        if (existing.id == theme.id) {
            existing = std::move(theme);
            return;
        }
    }
    themes_.append(std::move(theme));
}

const ThemeSpec *ThemeLibrary::byId(const QString &id) const
{
    for (const ThemeSpec &theme : themes_)
        if (theme.id == id)
            return &theme;
    return nullptr;
}

QVector<const ThemeSpec *> ThemeLibrary::byVariant(bool dark) const
{
    QVector<const ThemeSpec *> themes;
    for (const ThemeSpec &theme : themes_) {
        if (theme.dark == dark)
            themes.append(&theme);
    }
    return themes;
}

} // namespace osk
