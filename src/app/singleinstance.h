// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 d3npa <gh@w1t.ch>
#pragma once

#include <QObject>
#include <QStringList>

class QLocalServer;

namespace osk {

// Single-instance lock plus a command pipe: a second invocation forwards its
// arguments to the running instance and exits.
class SingleInstance : public QObject
{
    Q_OBJECT
public:
    enum ClaimResult { Primary, Secondary, Failed };

    explicit SingleInstance(QObject *parent = nullptr);

    ClaimResult claim(QString *error = nullptr);
    bool forward(const QStringList &args) const;

signals:
    void commandReceived(const QStringList &args);

private:
    QLocalServer *server_ = nullptr;
};

} // namespace osk
