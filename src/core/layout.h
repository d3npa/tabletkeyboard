// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 d3npa <gh@w1t.ch>
#pragma once

#include "core/sym_resolver.h"

#include <QJsonObject>
#include <QString>
#include <QStringList>
#include <QVector>

namespace osk {

// One key of a layout: a typed key, a sticky modifier, a command, or a gap.
struct KeyDef
{
    enum Type { Key, Mod, Action, Spacer };

    Type type = Key;
    QString label;
    double width = 1.0;

    QString sym;         // keysym name (type Key)
    quint32 symCode = 0; // resolved keysym
    QString shifted;     // keysym name sent while Shift is sticky/locked
    quint32 shiftedCode = 0;
    QString fn;          // keysym name sent while the OSK-local Fn modifier is armed
    quint32 fnCode = 0;
    QString fnLabel;     // label shown instead of `label` while Fn is armed

    QString mod;       // shift|ctrl|alt|super|altgr|fn (type Mod)
    QString action;    // hide|toggle_mode|toggle_lang|layer (type Action)
    QString layer;     // target layer for action "layer"
    QString indicator; // caps|num|scroll: visual state from the X server
    QString kana;      // printed kana legend, display only (type Key)
    bool repeat = true;

    // Stepped keys (JIS Return): `height` rows tall, `topWidth` units wide in
    // its first row; the rest of the key is `width` units wide, right-aligned.
    double height = 1.0;
    double topWidth = 0.0; // 0 = same as width
};

struct KeyRow
{
    QString id; // optional; a row whose id is hidden is skipped
    QVector<KeyDef> keys;

    // Width of the row in key units; stepped keys count their wider part.
    double widthUnits() const;
    bool isEmpty() const { return keys.isEmpty(); }
};

struct Block
{
    QString id; // optional; a block whose id is hidden is skipped
    QVector<KeyRow> rows;
    double topGap = 0.0; // vertical space before the block, in key units

    double widthUnits() const;
};

struct Layer
{
    QString name;
    QVector<Block> blocks;
};

struct Mode
{
    QString name;
    QVector<Layer> layers;

    const Layer *layer(const QString &name) const;
    const Layer *primaryLayer() const;
};

struct LayoutSet
{
    QString id;
    QString name;
    QVector<Mode> modes;

    const Mode *mode(const QString &name) const;
    const Mode *primaryMode() const;

    static bool fromJson(const QJsonObject &obj, const KeysymResolver *resolver, LayoutSet *out, QString *error);
    static bool loadFile(const QString &path, const KeysymResolver *resolver, LayoutSet *out, QString *error);
};

// Loads layout sets from, in increasing precedence: built-in resources
// (:/layouts), the system data dir, and the user's data dir.
class LayoutLibrary
{
public:
    explicit LayoutLibrary(const KeysymResolver *resolver);

    void scan();

    const QVector<LayoutSet> &sets() const { return sets_; }
    const LayoutSet *byId(const QString &id) const;
    int indexOf(const QString &id) const;
    const QStringList &errors() const { return errors_; }

private:
    void loadResourceDir(const QString &path);
    void loadDir(const QString &path);
    void insertOrReplace(LayoutSet &&set);

    const KeysymResolver *resolver_;
    QVector<LayoutSet> sets_;
    QStringList errors_;
};

} // namespace osk
