// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 d3npa <gh@w1t.ch>
#include "core/lint.h"
#include "core/keysyms.h"

#include <QFileInfo>
#include <QSet>

namespace osk {

QString LintIssue::toString() const
{
    const QString prefix = file.isEmpty() ? QString() : file + QStringLiteral(": ");
    const QString where = path.isEmpty() ? QString() : path + QStringLiteral(": ");
    return prefix + where
            + (severity == Error ? QStringLiteral("error: ") : QStringLiteral("warning: ")) + message;
}

namespace {

void addError(QVector<LintIssue> *issues, const QString &path, const QString &message)
{
    issues->append({ LintIssue::Error, QString(), path, message });
}

void addWarning(QVector<LintIssue> *issues, const QString &path, const QString &message)
{
    issues->append({ LintIssue::Warning, QString(), path, message });
}

bool keysymOk(const QString &name, const KeysymResolver *resolver, quint32 code)
{
    if (name.isEmpty())
        return false;
    if (resolver) {
        quint32 resolved = 0;
        return resolver->fromName(name, &resolved) && resolved != 0;
    }
    return code != 0;
}

} // namespace

QVector<LintIssue> Lint::check(const LayoutSet &set, const KeysymResolver *resolver)
{
    QVector<LintIssue> issues;

    if (set.id.isEmpty())
        addError(&issues, QStringLiteral("id"), QStringLiteral("layout id must not be empty"));
    if (set.name.isEmpty())
        addWarning(&issues, QStringLiteral("name"), QStringLiteral("layout name is empty"));
    if (set.modes.isEmpty())
        addError(&issues, QStringLiteral("modes"), QStringLiteral("layout has no modes"));

    for (const Mode &mode : set.modes) {
        const QString modePath = QStringLiteral("modes.") + mode.name;
        if (mode.layers.isEmpty())
            addError(&issues, modePath, QStringLiteral("mode has no layers"));

        QSet<QString> layerNames;
        for (const Layer &layer : mode.layers) {
            const QString layerPath = modePath + QStringLiteral(".layers.") + layer.name;
            if (layerNames.contains(layer.name))
                addError(&issues, layerPath, QStringLiteral("duplicate layer name"));
            layerNames.insert(layer.name);

            if (layer.name.isEmpty())
                addError(&issues, layerPath, QStringLiteral("layer name must not be empty"));
            if (layer.blocks.isEmpty())
                addError(&issues, layerPath, QStringLiteral("layer has no blocks"));

            for (int b = 0; b < layer.blocks.size(); ++b) {
                const Block &block = layer.blocks.at(b);
                if (block.rows.isEmpty())
                    addError(&issues, layerPath, QStringLiteral("block %1 has no rows").arg(b));
                if (block.topGap < 0)
                    addError(&issues, layerPath, QStringLiteral("block %1: topGap must be >= 0").arg(b));

                for (int r = 0; r < block.rows.size(); ++r) {
                    const KeyRow &row = block.rows.at(r);
                    if (row.isEmpty())
                        addError(&issues, layerPath, QStringLiteral("block %1 row %2 is empty").arg(b).arg(r));

                    for (int k = 0; k < row.keys.size(); ++k) {
                        const KeyDef &key = row.keys.at(k);
                        const QString keyPath = layerPath
                                + QStringLiteral("[%1][%2]").arg(r).arg(k);

                        if (!(key.width > 0))
                            addError(&issues, keyPath, QStringLiteral("width must be > 0"));
                        if (key.height < 1.0)
                            addError(&issues, keyPath, QStringLiteral("height must be >= 1"));
                        if (key.topWidth > key.width && k != row.keys.size() - 1)
                            addWarning(&issues, keyPath,
                                       QStringLiteral("a stepped key must be the last key of its row"));

                        switch (key.type) {
                        case KeyDef::Key:
                            if (key.sym.isEmpty())
                                addError(&issues, keyPath, QStringLiteral("key has no sym"));
                            else if (!keysymOk(key.sym, resolver, key.symCode))
                                addError(&issues, keyPath, QStringLiteral("unresolved keysym \"%1\"").arg(key.sym));
                            if (!key.shifted.isEmpty() && !keysymOk(key.shifted, resolver, key.shiftedCode))
                                addError(&issues, keyPath, QStringLiteral("unresolved keysym \"%1\"").arg(key.shifted));
                            if (key.label.isEmpty())
                                addWarning(&issues, keyPath, QStringLiteral("key has no label"));
                            break;
                        case KeyDef::Mod:
                            if (!isModifierId(key.mod))
                                addError(&issues, keyPath, QStringLiteral("unknown modifier \"%1\"").arg(key.mod));
                            break;
                        case KeyDef::Action:
                            if (key.action == QLatin1String("layer")) {
                                if (!layerNames.contains(key.layer) && !mode.layer(key.layer))
                                    addError(&issues, keyPath,
                                             QStringLiteral("action targets unknown layer \"%1\"").arg(key.layer));
                            } else if (key.action != QLatin1String("hide")
                                       && key.action != QLatin1String("toggle_mode")
                                       && key.action != QLatin1String("toggle_lang")) {
                                addError(&issues, keyPath, QStringLiteral("unknown action \"%1\"").arg(key.action));
                            }
                            break;
                        case KeyDef::Spacer:
                            break;
                        }

                        if (!key.indicator.isEmpty() && key.type != KeyDef::Key)
                            addWarning(&issues, keyPath, QStringLiteral("indicator only applies to type \"key\""));
                        if (!key.kana.isEmpty() && key.type != KeyDef::Key)
                            addWarning(&issues, keyPath, QStringLiteral("kana only applies to type \"key\""));
                    }
                }
            }
        }

        if (mode.layers.size() > 1 && !mode.layer(QStringLiteral("main")))
            addWarning(&issues, modePath, QStringLiteral("mode has several layers but none is named \"main\""));
    }

    return issues;
}

QVector<LintIssue> Lint::check(const ThemeSpec &theme)
{
    QVector<LintIssue> issues;

    if (theme.id.isEmpty())
        addError(&issues, QStringLiteral("id"), QStringLiteral("theme id must not be empty"));
    if (theme.name.isEmpty())
        addWarning(&issues, QStringLiteral("name"), QStringLiteral("theme name is empty"));

    for (const ThemeColorField &field : themeColorFields) {
        const QString &value = theme.*(field.member);
        if (field.optional && value.isEmpty())
            continue; // e.g. key_mid: the plain two-stop key gradient
        if (!isValidColor(value)) {
            addError(&issues, QStringLiteral("colors.") + QLatin1String(field.name),
                     QStringLiteral("must be #rrggbb"));
        }
    }

    if (!(theme.windowOpacity >= 0.0 && theme.windowOpacity <= 1.0))
        addError(&issues, QStringLiteral("colors.window_opacity"), QStringLiteral("must be within [0, 1]"));

    for (const ThemeMetricField &field : themeMetricFields) {
        const int value = theme.*(field.member);
        if (value < field.minValue) {
            addError(&issues, QStringLiteral("metrics.") + QLatin1String(field.name),
                     QStringLiteral("must be >= %1").arg(field.minValue));
        }
    }
    if (theme.fontFamily.isEmpty())
        addWarning(&issues, QStringLiteral("metrics.font_family"), QStringLiteral("font family is empty"));

    return issues;
}

QVector<LintIssue> Lint::checkLayoutFile(const QString &path, const KeysymResolver *resolver)
{
    LayoutSet set;
    QString error;
    if (!LayoutSet::loadFile(path, resolver, &set, &error)) {
        QVector<LintIssue> issues;
        addError(&issues, QString(), error);
        issues.first().file = QFileInfo(path).fileName();
        return issues;
    }

    QVector<LintIssue> issues = check(set, resolver);
    for (LintIssue &issue : issues)
        issue.file = QFileInfo(path).fileName();
    return issues;
}

bool Lint::hasErrors(const QVector<LintIssue> &issues)
{
    for (const LintIssue &issue : issues) {
        if (issue.severity == LintIssue::Error)
            return true;
    }
    return false;
}

} // namespace osk
