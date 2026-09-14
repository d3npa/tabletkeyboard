// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 d3npa <gh@w1t.ch>
#include "core/layout.h"
#include "core/keysyms.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QStandardPaths>

namespace osk {

namespace {

bool fail(QString *error, const QString &message)
{
    if (error)
        *error = message;
    return false;
}

bool parseKeyDef(const QJsonValue &value, const KeysymResolver *resolver, const QString &where, KeyDef *out, QString *error)
{
    if (!value.isObject())
        return fail(error, where + QStringLiteral(": key must be an object"));

    const QJsonObject o = value.toObject();
    KeyDef key;

    const QString type = o.value(QStringLiteral("type")).toString(QStringLiteral("key"));
    if (type == QLatin1String("key"))
        key.type = KeyDef::Key;
    else if (type == QLatin1String("mod"))
        key.type = KeyDef::Mod;
    else if (type == QLatin1String("action"))
        key.type = KeyDef::Action;
    else if (type == QLatin1String("spacer"))
        key.type = KeyDef::Spacer;
    else
        return fail(error, where + QStringLiteral(": unknown key type \"%1\"").arg(type));

    key.label = o.value(QStringLiteral("label")).toString();
    key.width = o.value(QStringLiteral("width")).toDouble(1.0);
    if (!(key.width > 0))
        return fail(error, where + QStringLiteral(": width must be > 0"));

    key.repeat = o.value(QStringLiteral("repeat")).toBool(true);
    key.indicator = o.value(QStringLiteral("indicator")).toString();
    if (!key.indicator.isEmpty() && key.indicator != QLatin1String("caps") && key.indicator != QLatin1String("num")
        && key.indicator != QLatin1String("scroll"))
        return fail(error, where + QStringLiteral(": unknown indicator \"%1\"").arg(key.indicator));
    if (!key.indicator.isEmpty())
        key.repeat = false; // never auto-repeat a lock key

    key.height = o.value(QStringLiteral("height")).toDouble(1.0);
    if (key.height < 1.0)
        return fail(error, where + QStringLiteral(": height must be >= 1"));
    if (o.contains(QStringLiteral("topWidth"))) {
        key.topWidth = o.value(QStringLiteral("topWidth")).toDouble(0.0);
        if (!(key.topWidth > 0))
            return fail(error, where + QStringLiteral(": topWidth must be > 0"));
    }

    switch (key.type) {
    case KeyDef::Key: {
        key.sym = o.value(QStringLiteral("sym")).toString();
        if (key.sym.isEmpty())
            return fail(error, where + QStringLiteral(": key needs a \"sym\""));
        key.kana = o.value(QStringLiteral("kana")).toString();
        if (resolver && !resolver->fromName(key.sym, &key.symCode))
            return fail(error, where + QStringLiteral(": unknown keysym \"%1\"").arg(key.sym));
        key.shifted = o.value(QStringLiteral("shifted")).toString();
        if (!key.shifted.isEmpty() && resolver && !resolver->fromName(key.shifted, &key.shiftedCode))
            return fail(error, where + QStringLiteral(": unknown keysym \"%1\"").arg(key.shifted));
        key.fn = o.value(QStringLiteral("fn")).toString();
        if (!key.fn.isEmpty() && resolver && !resolver->fromName(key.fn, &key.fnCode))
            return fail(error, where + QStringLiteral(": unknown keysym \"%1\"").arg(key.fn));
        key.fnLabel = o.value(QStringLiteral("fnLabel")).toString();
        if (!key.fn.isEmpty() && key.fnLabel.isEmpty()) {
            key.fnLabel = displayTextForKeysym(key.fnCode);
            if (key.fnLabel.isEmpty())
                key.fnLabel = key.fn;
        }
        if (key.label.isEmpty())
            key.label = displayTextForKeysym(key.symCode);
        if (key.label.isEmpty())
            key.label = key.sym;
        break;
    }
    case KeyDef::Mod: {
        key.mod = o.value(QStringLiteral("mod")).toString();
        if (!isModifierId(key.mod))
            return fail(error, where + QStringLiteral(": unknown modifier \"%1\"").arg(key.mod));
        if (key.label.isEmpty())
            key.label = key.mod;
        break;
    }
    case KeyDef::Action: {
        key.action = o.value(QStringLiteral("action")).toString();
        if (key.action != QLatin1String("hide") && key.action != QLatin1String("toggle_mode")
            && key.action != QLatin1String("toggle_lang") && key.action != QLatin1String("layer"))
            return fail(error, where + QStringLiteral(": unknown action \"%1\"").arg(key.action));
        if (key.action == QLatin1String("layer")) {
            key.layer = o.value(QStringLiteral("layer")).toString();
            if (key.layer.isEmpty())
                return fail(error, where + QStringLiteral(": action \"layer\" needs a \"layer\" name"));
        }
        if (key.label.isEmpty())
            key.label = key.action;
        break;
    }
    case KeyDef::Spacer:
        break;
    }

    *out = key;
    return true;
}

bool parseBlock(const QJsonValue &value, const KeysymResolver *resolver, const QString &where, Block *out, QString *error)
{
    if (!value.isObject())
        return fail(error, where + QStringLiteral(": block must be an object"));
    const QJsonObject o = value.toObject();

    Block block;
    block.id = o.value(QStringLiteral("id")).toString();
    block.topGap = o.value(QStringLiteral("topGap")).toDouble(0.0);
    if (block.topGap < 0)
        return fail(error, where + QStringLiteral(": topGap must be >= 0"));

    const QJsonArray rows = o.value(QStringLiteral("rows")).toArray();
    if (rows.isEmpty())
        return fail(error, where + QStringLiteral(": block needs \"rows\""));

    for (int r = 0; r < rows.size(); ++r) {
        KeyRow row;
        QString keysWhere = where + QStringLiteral(".rows[%1]").arg(r);
        QJsonArray rowArray;
        if (rows.at(r).isObject()) {
            const QJsonObject rowObject = rows.at(r).toObject();
            row.id = rowObject.value(QStringLiteral("id")).toString();
            keysWhere += QStringLiteral(".keys");
            rowArray = rowObject.value(QStringLiteral("keys")).toArray();
        } else {
            rowArray = rows.at(r).toArray();
        }
        if (rowArray.isEmpty())
            return fail(error, keysWhere + QStringLiteral(": row must be a non-empty array"));
        for (int k = 0; k < rowArray.size(); ++k) {
            KeyDef key;
            const QString keyWhere = keysWhere + QStringLiteral("[%1]").arg(k);
            if (!parseKeyDef(rowArray.at(k), resolver, keyWhere, &key, error))
                return false;
            row.keys.append(key);
        }
        block.rows.append(row);
    }

    *out = block;
    return true;
}

bool parseLayer(const QJsonValue &value, const KeysymResolver *resolver, const QString &where, Layer *out, QString *error)
{
    if (!value.isObject())
        return fail(error, where + QStringLiteral(": layer must be an object"));
    const QJsonObject o = value.toObject();

    Layer layer;
    layer.name = o.value(QStringLiteral("name")).toString();
    if (layer.name.isEmpty())
        return fail(error, where + QStringLiteral(": layer needs a \"name\""));

    const QJsonArray blocks = o.value(QStringLiteral("blocks")).toArray();
    if (blocks.isEmpty())
        return fail(error, where + QStringLiteral(": layer needs \"blocks\""));
    for (int b = 0; b < blocks.size(); ++b) {
        Block block;
        if (!parseBlock(blocks.at(b), resolver, where + QStringLiteral(".blocks[%1]").arg(b), &block, error))
            return false;
        layer.blocks.append(block);
    }

    *out = layer;
    return true;
}

bool parseMode(const QString &name, const QJsonValue &value, const KeysymResolver *resolver, Mode *out, QString *error)
{
    const QString where = QStringLiteral("modes.") + name;
    if (!value.isObject())
        return fail(error, where + QStringLiteral(": mode must be an object"));
    const QJsonObject o = value.toObject();

    Mode mode;
    mode.name = name;
    const QJsonArray layers = o.value(QStringLiteral("layers")).toArray();
    if (layers.isEmpty())
        return fail(error, where + QStringLiteral(": mode needs \"layers\""));
    for (int l = 0; l < layers.size(); ++l) {
        Layer layer;
        if (!parseLayer(layers.at(l), resolver, where + QStringLiteral(".layers[%1]").arg(l), &layer, error))
            return false;
        mode.layers.append(layer);
    }

    *out = mode;
    return true;
}

} // namespace

double KeyRow::widthUnits() const
{
    double sum = 0;
    for (const KeyDef &key : keys)
        sum += qMax(key.width, key.topWidth);
    return sum;
}

double Block::widthUnits() const
{
    double w = 0;
    for (const KeyRow &row : rows)
        w = qMax(w, row.widthUnits());
    return w;
}

const Layer *Mode::layer(const QString &layerName) const
{
    for (const Layer &l : layers)
        if (l.name == layerName)
            return &l;
    return nullptr;
}

const Layer *Mode::primaryLayer() const
{
    return layers.isEmpty() ? nullptr : &layers.first();
}

const Mode *LayoutSet::mode(const QString &modeName) const
{
    for (const Mode &m : modes)
        if (m.name == modeName)
            return &m;
    return nullptr;
}

const Mode *LayoutSet::primaryMode() const
{
    return modes.isEmpty() ? nullptr : &modes.first();
}

bool LayoutSet::fromJson(const QJsonObject &obj, const KeysymResolver *resolver, LayoutSet *out, QString *error)
{
    LayoutSet set;
    set.id = obj.value(QStringLiteral("id")).toString();
    if (set.id.isEmpty())
        return fail(error, QStringLiteral("layout: missing \"id\""));
    set.name = obj.value(QStringLiteral("name")).toString(set.id);

    const QJsonValue modesValue = obj.value(QStringLiteral("modes"));
    if (!modesValue.isObject() || modesValue.toObject().isEmpty())
        return fail(error, QStringLiteral("layout %1: missing \"modes\"").arg(set.id));

    const QJsonObject modes = modesValue.toObject();
    for (auto it = modes.begin(); it != modes.end(); ++it) {
        Mode mode;
        if (!parseMode(it.key(), it.value(), resolver, &mode, error))
            return false;
        set.modes.append(mode);
    }

    if (set.modes.isEmpty())
        return fail(error, QStringLiteral("layout %1: no modes").arg(set.id));

    *out = set;
    return true;
}

bool LayoutSet::loadFile(const QString &path, const KeysymResolver *resolver, LayoutSet *out, QString *error)
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

    if (!fromJson(doc.object(), resolver, out, error))
        return fail(error, QStringLiteral("%1: %2").arg(path, error ? *error : QString()));

    if (out->id != QFileInfo(path).completeBaseName())
        return fail(error, QStringLiteral("%1: id \"%2\" must match the file name").arg(path, out->id));

    return true;
}

LayoutLibrary::LayoutLibrary(const KeysymResolver *resolver) : resolver_(resolver) {}

void LayoutLibrary::scan()
{
    sets_.clear();
    errors_.clear();

    loadResourceDir(QStringLiteral(":/layouts"));
    loadDir(QStringLiteral("/usr/local/share/tabletkeyboard/layouts"));
    loadDir(QStringLiteral("/usr/share/tabletkeyboard/layouts"));
    loadDir(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
            + QStringLiteral("/tabletkeyboard/layouts"));
}

void LayoutLibrary::loadResourceDir(const QString &path)
{
    const QDir dir(path);
    const QStringList files = dir.entryList({ QStringLiteral("*.json") }, QDir::Files, QDir::Name);
    for (const QString &file : files) {
        LayoutSet set;
        QString error;
        if (!LayoutSet::loadFile(dir.filePath(file), resolver_, &set, &error))
            errors_.append(error);
        else
            insertOrReplace(std::move(set));
    }
}

void LayoutLibrary::loadDir(const QString &path)
{
    const QDir dir(path);
    if (!dir.exists())
        return;
    const QStringList files = dir.entryList({ QStringLiteral("*.json") }, QDir::Files, QDir::Name);
    for (const QString &file : files) {
        LayoutSet set;
        QString error;
        if (!LayoutSet::loadFile(dir.filePath(file), resolver_, &set, &error))
            errors_.append(error);
        else
            insertOrReplace(std::move(set));
    }
}

void LayoutLibrary::insertOrReplace(LayoutSet &&set)
{
    for (LayoutSet &existing : sets_) {
        if (existing.id == set.id) {
            existing = std::move(set);
            return;
        }
    }
    sets_.append(std::move(set));
}

const LayoutSet *LayoutLibrary::byId(const QString &id) const
{
    for (const LayoutSet &set : sets_)
        if (set.id == id)
            return &set;
    return nullptr;
}

int LayoutLibrary::indexOf(const QString &id) const
{
    for (int i = 0; i < sets_.size(); ++i)
        if (sets_.at(i).id == id)
            return i;
    return -1;
}

} // namespace osk
