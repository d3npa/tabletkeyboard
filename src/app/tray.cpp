// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 d3npa <gh@w1t.ch>
#include "app/tray.h"

#include "app/app.h"
#include "app/autostart.h"
#include "app/icon.h"
#include "core/blocks.h"
#include "core/layout.h"
#include "core/theme.h"

#include <QAction>
#include <QCoreApplication>
#include <QMenu>
#include <QSystemTrayIcon>

namespace osk {

Tray::Tray(App *app, QObject *parent) : QObject(parent), app_(app)
{
    if (!QSystemTrayIcon::isSystemTrayAvailable())
        return;

    tray_ = new QSystemTrayIcon(appIcon(), this);
    menu_ = new QMenu();
    tray_->setContextMenu(menu_);
    tray_->setToolTip(QStringLiteral("tabletkeyboard"));
    connect(menu_, &QMenu::aboutToShow, this, &Tray::rebuildMenu);
    connect(tray_, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger)
            app_->toggleKeyboard();
    });
    tray_->show();
}

Tray::~Tray()
{
    // QSystemTrayIcon::setContextMenu() does not take ownership, and the menu
    // has no widget parent either.
    delete menu_;
}

void Tray::rebuildMenu()
{
    menu_->clear();

    QAction *toggle = menu_->addAction(app_->isKeyboardVisible() ? QStringLiteral("Hide keyboard") : QStringLiteral("Show keyboard"));
    connect(toggle, &QAction::triggered, app_, &App::toggleKeyboard);

    const LayoutSet *layout = app_->machine()->layout();
    if (layout && layout->modes.size() > 1) {
        QMenu *modeMenu = menu_->addMenu(QStringLiteral("Mode"));
        for (const Mode &mode : layout->modes) {
            QAction *action = modeMenu->addAction(mode.name);
            action->setCheckable(true);
            action->setChecked(mode.name == app_->machine()->modeId());
            const QString id = mode.name;
            connect(action, &QAction::triggered, this, [this, id]() { app_->setModeId(id); });
        }
    }

    if (app_->layouts()->sets().size() > 1) {
        QMenu *languageMenu = menu_->addMenu(QStringLiteral("Language"));
        for (const LayoutSet &set : app_->layouts()->sets()) {
            QAction *action = languageMenu->addAction(set.name);
            action->setCheckable(true);
            action->setChecked(set.id == app_->machine()->layoutId());
            const QString id = set.id;
            connect(action, &QAction::triggered, this, [this, id]() { app_->setLayoutId(id); });
        }
    }

    QAction *dark = menu_->addAction(QStringLiteral("Dark mode"));
    dark->setCheckable(true);
    dark->setChecked(app_->settings().darkMode); // setChecked before connect: no handler on rebuild
    connect(dark, &QAction::toggled, app_, &App::setDarkMode);

    QMenu *blocksMenu = menu_->addMenu(QStringLiteral("Blocks"));
    QAction *frow = blocksMenu->addAction(QStringLiteral("F-row"));
    frow->setCheckable(true);
    frow->setChecked(app_->settings().showFrow);
    connect(frow, &QAction::toggled, this,
            [this](bool on) { app_->setBlockVisible(QString::fromLatin1(blocks::kFrow), on); });
    QAction *numpad = blocksMenu->addAction(QStringLiteral("Numpad"));
    numpad->setCheckable(true);
    numpad->setChecked(app_->settings().showNumpad);
    connect(numpad, &QAction::toggled, this,
            [this](bool on) { app_->setBlockVisible(QString::fromLatin1(blocks::kNumpad), on); });

    if (app_->themes()->themes().size() > 1) {
        QMenu *themeMenu = menu_->addMenu(QStringLiteral("Theme"));
        for (const ThemeSpec *theme : app_->themes()->byVariant(app_->settings().darkMode)) {
            QAction *action = themeMenu->addAction(theme->name);
            action->setCheckable(true);
            action->setChecked(theme->id == app_->settings().themeId());
            const QString id = theme->id;
            connect(action, &QAction::triggered, this, [this, id]() { app_->setThemeId(id); });
        }
    }

    QMenu *scaleMenu = menu_->addMenu(QStringLiteral("Scale"));
    for (const double scale : { 0.8, 1.0, 1.25, 1.5, 2.0 }) {
        QAction *action = scaleMenu->addAction(QStringLiteral("%1%").arg(qRound(scale * 100)));
        action->setCheckable(true);
        action->setChecked(qFuzzyCompare(app_->scale(), scale));
        connect(action, &QAction::triggered, this, [this, scale]() { app_->setScale(scale); });
    }

    QAction *autostart = menu_->addAction(QStringLiteral("Start at login"));
    autostart->setCheckable(true);
    autostart->setChecked(app_->autostartEnabled());
    connect(autostart, &QAction::toggled, app_, &App::setAutostart);

    menu_->addSeparator();
    QAction *settings = menu_->addAction(QStringLiteral("Settings…"));
    connect(settings, &QAction::triggered, app_, &App::showSettings);
    menu_->addAction(QStringLiteral("Quit"), qApp, &QCoreApplication::quit);
}

} // namespace osk
