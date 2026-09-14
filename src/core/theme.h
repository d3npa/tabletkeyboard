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
    bool dark = false; // false = light variant; the Light/Dark pickers list matching themes only

    QString keyTop = QStringLiteral("#fdfdfd");
    QString keyBottom = QStringLiteral("#f0f0f0");
    QString keyMid; // optional middle gradient stop for a metallic sheen (empty = two stops)
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
    QString ledOn = QStringLiteral("#4caf50");
    QString ledOff = QStringLiteral("#8a8a8a");

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

// The JSON fields of a theme, shared by ThemeSpec::fromJson() and
// Lint::check(): a colour or metric added here is parsed and validated in one
// place, so a forgotten field cannot slip past the linter.
struct ThemeColorField
{
    const char *name; // JSON key inside "colors"
    QString ThemeSpec::*member;
    bool optional = false; // an empty value is allowed (key_mid)
};

struct ThemeMetricField
{
    const char *name; // JSON key inside "metrics"
    int ThemeSpec::*member;
    int minValue;
};

inline constexpr ThemeColorField themeColorFields[] = {
    { "key_top", &ThemeSpec::keyTop },
    { "key_bottom", &ThemeSpec::keyBottom },
    { "key_mid", &ThemeSpec::keyMid, true },
    { "key_border", &ThemeSpec::keyBorder },
    { "key_text", &ThemeSpec::keyText },
    { "key_pressed_top", &ThemeSpec::keyPressedTop },
    { "key_pressed_bottom", &ThemeSpec::keyPressedBottom },
    { "key_hover", &ThemeSpec::keyHover },
    { "mod_active", &ThemeSpec::modActive },
    { "accent", &ThemeSpec::accent },
    { "bar_bg", &ThemeSpec::barBg },
    { "window_bg", &ThemeSpec::windowBg },
    { "led_on", &ThemeSpec::ledOn },
    { "led_off", &ThemeSpec::ledOff },
};

inline constexpr ThemeMetricField themeMetricFields[] = {
    { "radius", &ThemeSpec::radius, 0 },
    { "border", &ThemeSpec::border, 0 },
    { "gap", &ThemeSpec::gap, 0 },
    { "padding", &ThemeSpec::padding, 0 },
    { "key_unit", &ThemeSpec::keyUnit, 8 },
    { "bar_height", &ThemeSpec::barHeight, 8 },
    { "font_px", &ThemeSpec::fontPx, 6 },
    { "label_px", &ThemeSpec::labelPx, 5 },
};

class ThemeLibrary
{
public:
    ThemeLibrary() = default;

    void scan();

    const QVector<ThemeSpec> &themes() const { return themes_; }
    const ThemeSpec *byId(const QString &id) const;
    QVector<const ThemeSpec *> byVariant(bool dark) const; // scan order (file-name order)
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
